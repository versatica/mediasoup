use crate::messages::{MetaNotification, MetaRequest, Notification, Request};
use crate::worker::common::{EventHandlers, SubscriptionTarget, WeakEventHandlers};
use crate::worker::{RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_fs::File;
use bytes::Bytes;
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncReadExt, AsyncWriteExt};
use log::{debug, error, trace, warn};
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt::Debug;
use std::io;
use std::sync::{Arc, Weak};
use std::time::Duration;
use thiserror::Error;

const PAYLOAD_MAX_LEN: usize = 4_194_304;

#[derive(Debug)]
pub(super) enum InternalMessage {
    /// Unknown data
    UnexpectedData(Vec<u8>),
}

enum RequestOrNotificationWithPayload {
    Request {
        id: u32,
        request: MetaRequest,
        payload: Bytes,
    },
    Notification {
        notification: MetaNotification,
        payload: Bytes,
    },
}

#[derive(Clone)]
pub(crate) struct NotificationMessage {
    pub(crate) message: Value,
    pub(crate) payload: Bytes,
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum PayloadChannelReceiveMessage {
    ResponseSuccess {
        id: u32,
        accepted: bool,
        data: Option<Value>,
    },
    ResponseError {
        id: u32,
        error: Value,
        reason: String,
    },
    Notification(Value),
    /// Unknown data
    #[serde(skip)]
    Internal(InternalMessage),
}

fn deserialize_message(bytes: &[u8]) -> PayloadChannelReceiveMessage {
    match serde_json::from_slice(bytes) {
        Ok(message) => message,
        Err(error) => {
            warn!("Failed to deserialize message: {}", error);

            PayloadChannelReceiveMessage::Internal(InternalMessage::UnexpectedData(Vec::from(
                bytes,
            )))
        }
    }
}

#[derive(Debug, Error, Eq, PartialEq)]
pub enum NotificationError {
    #[error("Channel already closed")]
    ChannelClosed,
    #[error("Payload is too long")]
    PayloadTooLong,
}

struct ResponseError {
    reason: String,
}

type Response<T> = Result<Option<T>, ResponseError>;

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: HashMap<u32, async_oneshot::Sender<Response<Value>>>,
}

struct Inner {
    sender: async_channel::Sender<RequestOrNotificationWithPayload>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container_weak: Weak<Mutex<RequestsContainer>>,
    event_handlers_weak: WeakEventHandlers<NotificationMessage>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        self.sender.close();
        self.internal_message_receiver.close();
    }
}

#[derive(Clone)]
pub(crate) struct PayloadChannel {
    inner: Arc<Inner>,
}

impl PayloadChannel {
    pub(super) fn new(executor: &Executor<'static>, reader: File, mut writer: File) -> Self {
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let requests_container_weak = Arc::downgrade(&requests_container);
        let event_handlers = EventHandlers::new();
        let event_handlers_weak = event_handlers.downgrade();

        let internal_message_receiver = {
            let (sender, receiver) = async_channel::bounded(1);

            executor
                .spawn(async move {
                    let mut len_bytes = [0u8; 4];
                    let mut bytes = Vec::with_capacity(PAYLOAD_MAX_LEN);
                    let mut reader = BufReader::new(reader);

                    loop {
                        reader.read_exact(&mut len_bytes).await?;
                        let length = u32::from_ne_bytes(len_bytes) as usize;

                        if bytes.len() < length {
                            // Increase bytes size if/when needed
                            bytes.resize(length, 0);
                        }
                        reader.read_exact(&mut bytes[..length]).await?;

                        trace!(
                            "received raw message: {}",
                            String::from_utf8_lossy(&bytes[..length]),
                        );

                        match deserialize_message(&bytes[..length]) {
                            PayloadChannelReceiveMessage::ResponseSuccess {
                                id,
                                accepted: _,
                                data,
                            } => {
                                let sender = requests_container.lock().handlers.remove(&id);
                                if let Some(mut sender) = sender {
                                    let _ = sender.send(Ok(data));
                                } else {
                                    warn!(
                                        "received success response does not match any sent request \
                                         [id:{}]",
                                        id,
                                    );
                                }
                            }
                            PayloadChannelReceiveMessage::ResponseError {
                                id,
                                error: _,
                                reason,
                            } => {
                                let sender = requests_container.lock().handlers.remove(&id);
                                if let Some(mut sender) = sender {
                                    let _ = sender.send(Err(ResponseError { reason }));
                                } else {
                                    warn!(
                                        "received error response does not match any sent request \
                                        [id:{}]",
                                        id,
                                    );
                                }
                            }
                            PayloadChannelReceiveMessage::Notification(notification) => {
                                let target_id = notification.get("targetId").and_then(|value| {
                                    let str = value.as_str()?;
                                    str.parse().ok().map(SubscriptionTarget::Uuid).or_else(|| {
                                        str.parse().ok().map(SubscriptionTarget::Number)
                                    })
                                });

                                reader.read_exact(&mut len_bytes).await?;
                                let length = u32::from_ne_bytes(len_bytes) as usize;

                                if bytes.len() < length {
                                    // Increase bytes size if/when needed
                                    bytes.resize(length, 0);
                                }
                                reader.read_exact(&mut bytes[..length]).await?;

                                trace!("received notification payload of {} bytes", length);

                                let payload = Bytes::copy_from_slice(&bytes[..length]);

                                if let Some(target_id) = target_id {
                                    event_handlers.call_callbacks_with_value(
                                        &target_id,
                                        NotificationMessage {
                                            message: notification,
                                            payload,
                                        },
                                    );
                                } else {
                                    let unexpected_message = InternalMessage::UnexpectedData(
                                        Vec::from(&bytes[..length]),
                                    );
                                    if sender.send(unexpected_message).await.is_err() {
                                        break;
                                    }
                                }
                            }
                            PayloadChannelReceiveMessage::Internal(internal_message) => {
                                if sender.send(internal_message).await.is_err() {
                                    break;
                                }
                            }
                        }
                    }

                    io::Result::Ok(())
                })
                .detach();

            receiver
        };

        let sender = {
            let (sender, receiver) = async_channel::bounded::<RequestOrNotificationWithPayload>(1);

            executor
                .spawn(async move {
                    let mut write_buffer = Vec::with_capacity(PAYLOAD_MAX_LEN);

                    while let Ok(message) = receiver.recv().await {
                        write_buffer.resize(4, 0);

                        match message {
                            RequestOrNotificationWithPayload::Request {
                                id,
                                request,
                                payload,
                            } => {
                                #[derive(Debug, Serialize)]
                                struct RequestMessagePrivate<'a, R> {
                                    id: u32,
                                    method: &'static str,
                                    #[serde(flatten)]
                                    request: &'a R,
                                }

                                serde_json::to_writer(
                                    &mut write_buffer[4..],
                                    &RequestMessagePrivate {
                                        id,
                                        method: request.as_method(),
                                        request: &request,
                                    },
                                )
                                .unwrap();

                                {
                                    let len = write_buffer.len() as u32 - 4;
                                    write_buffer[..4].copy_from_slice(&len.to_ne_bytes());
                                }

                                write_buffer
                                    .extend_from_slice(&(payload.len() as u32).to_ne_bytes());

                                writer.write_all(&write_buffer).await?;

                                writer.write_all(&payload).await?;
                            }
                            RequestOrNotificationWithPayload::Notification {
                                notification,
                                payload,
                            } => {
                                #[derive(Debug, Serialize)]
                                struct NotificationMessagePrivate<'a, N: Serialize> {
                                    event: &'static str,
                                    #[serde(flatten)]
                                    notification: &'a N,
                                }

                                serde_json::to_writer(
                                    &mut write_buffer,
                                    &NotificationMessagePrivate {
                                        event: notification.as_event(),
                                        notification: &notification,
                                    },
                                )
                                .unwrap();

                                {
                                    let len = write_buffer.len() as u32 - 4;
                                    write_buffer[..4].copy_from_slice(&len.to_ne_bytes());
                                }

                                write_buffer
                                    .extend_from_slice(&(payload.len() as u32).to_ne_bytes());

                                writer.write_all(&write_buffer).await?;

                                // Payload is written directly as it can be relatively large
                                writer.write_all(&payload).await?;
                            }
                        }
                    }

                    io::Result::Ok(())
                })
                .detach();

            sender
        };

        let inner = Arc::new(Inner {
            sender,
            internal_message_receiver,
            requests_container_weak,
            event_handlers_weak,
        });

        Self { inner }
    }

    pub(super) fn get_internal_message_receiver(&self) -> async_channel::Receiver<InternalMessage> {
        self.inner.internal_message_receiver.clone()
    }

    pub(crate) async fn request<R>(
        &self,
        request: R,
        payload: Bytes,
    ) -> Result<R::Response, RequestError>
    where
        R: Request + Into<MetaRequest>,
    {
        let method = request.as_method();

        let id;
        let queue_len;
        let (result_sender, result_receiver) = async_oneshot::oneshot();

        {
            let requests_container_lock = self
                .inner
                .requests_container_weak
                .upgrade()
                .ok_or(RequestError::ChannelClosed)?;
            let mut requests_container = requests_container_lock.lock();

            id = requests_container.next_id;
            queue_len = requests_container.handlers.len();

            requests_container.next_id = requests_container.next_id.wrapping_add(1);
            requests_container.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]: {:?}", method, id, request);

        if payload.len() > PAYLOAD_MAX_LEN {
            self.inner
                .requests_container_weak
                .upgrade()
                .ok_or(RequestError::ChannelClosed)?
                .lock()
                .handlers
                .remove(&id);
            return Err(RequestError::PayloadTooLong);
        }

        self.inner
            .sender
            .send(RequestOrNotificationWithPayload::Request {
                id,
                request: request.into(),
                payload,
            })
            .await
            .map_err(|_| RequestError::ChannelClosed {})?;

        let result = future::or(
            async move {
                result_receiver
                    .await
                    .map_err(|_| RequestError::ChannelClosed {})
            },
            async move {
                async_io::Timer::after(Duration::from_millis(
                    (1000.0 * (15.0 + (0.1 * queue_len as f64))).round() as u64,
                ))
                .await;

                if let Some(requests_container) = self.inner.requests_container_weak.upgrade() {
                    requests_container.lock().handlers.remove(&id);
                }

                Err(RequestError::TimedOut)
            },
        )
        .await?;

        let data = match result {
            Ok(data) => {
                debug!("request succeeded [method:{}, id:{}]", method, id);
                data
            }
            Err(ResponseError { reason }) => {
                debug!("request failed [method:{}, id:{}]: {}", method, id, reason);

                return Err(RequestError::Response { reason });
            }
        };

        // Default will work for `()` response
        serde_json::from_value(data.unwrap_or_default()).map_err(|error| {
            RequestError::FailedToParse {
                error: error.to_string(),
            }
        })
    }

    pub(crate) async fn notify<N>(
        &self,
        notification: N,
        payload: Bytes,
    ) -> Result<(), NotificationError>
    where
        N: Notification + Into<MetaNotification>,
    {
        debug!("notify() [event:{}]", notification.as_event());

        if payload.len() > PAYLOAD_MAX_LEN {
            return Err(NotificationError::PayloadTooLong);
        }

        self.inner
            .sender
            .send(RequestOrNotificationWithPayload::Notification {
                notification: notification.into(),
                payload,
            })
            .await
            .map_err(|_| NotificationError::ChannelClosed {})
    }

    pub(crate) fn subscribe_to_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> Option<SubscriptionHandler>
    where
        F: Fn(NotificationMessage) + Send + Sync + 'static,
    {
        self.inner
            .event_handlers_weak
            .upgrade()
            .map(|event_handlers| event_handlers.add(target_id, Box::new(callback)))
    }
}
