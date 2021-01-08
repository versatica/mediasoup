use crate::messages::Request;
use crate::worker::common::{EventHandlers, SubscriptionTarget};
use crate::worker::{RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_fs::File;
use bytes::{BufMut, Bytes, BytesMut};
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use log::*;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt::Debug;
use std::io;
use std::io::Write;
use std::sync::Arc;
use std::time::Duration;

const NS_PAYLOAD_MAX_LEN: usize = 4_194_304;

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub(super) enum InternalMessage {
    /// Debug log
    #[serde(skip)]
    Debug(String),
    /// Warn log
    #[serde(skip)]
    Warn(String),
    /// Error log
    #[serde(skip)]
    Error(String),
    /// Dump log
    #[serde(skip)]
    Dump(String),
    /// Unknown
    #[serde(skip)]
    Unexpected(Vec<u8>),
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum ChannelReceiveMessage {
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
    Event(InternalMessage),
}

fn deserialize_message(bytes: &[u8]) -> ChannelReceiveMessage {
    match bytes[0] {
        // JSON message
        b'{' => serde_json::from_slice(bytes).unwrap(),
        // Debug log
        b'D' => ChannelReceiveMessage::Event(InternalMessage::Debug(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Warn log
        b'W' => ChannelReceiveMessage::Event(InternalMessage::Warn(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Error log
        b'E' => ChannelReceiveMessage::Event(InternalMessage::Error(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Dump log
        b'X' => ChannelReceiveMessage::Event(InternalMessage::Dump(
            String::from_utf8(Vec::from(&bytes[1..])).unwrap(),
        )),
        // Unknown
        _ => ChannelReceiveMessage::Event(InternalMessage::Unexpected(Vec::from(bytes))),
    }
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
    sender: async_channel::Sender<Bytes>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container: Arc<Mutex<RequestsContainer>>,
    event_handlers: EventHandlers<Value>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        self.sender.close();
        self.internal_message_receiver.close();
    }
}

#[derive(Clone)]
pub(crate) struct Channel {
    inner: Arc<Inner>,
}

impl Channel {
    pub(super) fn new(executor: Arc<Executor<'static>>, reader: File, mut writer: File) -> Self {
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let event_handlers = EventHandlers::new(Arc::clone(&executor));

        let internal_message_receiver = {
            let requests_container = Arc::clone(&requests_container);
            let event_handlers = event_handlers.clone();
            let (sender, receiver) = async_channel::bounded(1);

            executor
                .spawn(async move {
                    let mut len_bytes = Vec::new();
                    let mut bytes = Vec::new();
                    let mut reader = BufReader::new(reader);

                    loop {
                        let read_bytes = reader.read_until(b':', &mut len_bytes).await?;
                        if read_bytes == 0 {
                            // EOF
                            break;
                        }
                        let length = String::from_utf8_lossy(&len_bytes[..(read_bytes - 1)])
                            .parse::<usize>()
                            .unwrap();

                        if length > NS_PAYLOAD_MAX_LEN {
                            warn!(
                                "received message {} is too long, max supported is {}",
                                length, NS_PAYLOAD_MAX_LEN,
                            );
                        }

                        len_bytes.clear();
                        if bytes.len() < length + 1 {
                            // Increase bytes size if/when needed
                            bytes.resize(length + 1, 0);
                        }
                        // +1 because of netstring `,` at the very end
                        reader.read_exact(&mut bytes[..(length + 1)]).await?;

                        trace!(
                            "received raw message: {}",
                            String::from_utf8_lossy(&bytes[..length]),
                        );

                        match deserialize_message(&bytes[..length]) {
                            ChannelReceiveMessage::ResponseSuccess {
                                id,
                                accepted: _,
                                data,
                            } => {
                                let sender = requests_container.lock().handlers.remove(&id);
                                if let Some(sender) = sender {
                                    let _ = sender.send(Ok(data));
                                } else {
                                    warn!(
                                        "received success response does not match any sent request \
                                        [id:{}]",
                                        id,
                                    );
                                }
                            }
                            ChannelReceiveMessage::ResponseError {
                                id,
                                error: _,
                                reason,
                            } => {
                                let sender = requests_container.lock().handlers.remove(&id);
                                if let Some(sender) = sender {
                                    let _ = sender.send(Err(ResponseError { reason }));
                                } else {
                                    warn!(
                                        "received error response does not match any sent request \
                                        [id:{}]",
                                        id,
                                    );
                                }
                            }
                            ChannelReceiveMessage::Notification(notification) => {
                                let target_id = notification.get("targetId").and_then(|value| {
                                    let str = value.as_str()?;
                                    str.parse().ok().map(SubscriptionTarget::Uuid).or_else(|| {
                                        str.parse().ok().map(SubscriptionTarget::Number)
                                    })
                                });

                                match target_id {
                                    Some(target_id) => {
                                        event_handlers
                                            .call_callbacks_with_value(&target_id, notification);
                                    }
                                    None => {
                                        let unexpected_message = InternalMessage::Unexpected(
                                            Vec::from(&bytes[..length]),
                                        );
                                        if sender.send(unexpected_message).await.is_err() {
                                            break;
                                        }
                                    }
                                }
                            }
                            ChannelReceiveMessage::Event(event_message) => {
                                if sender.send(event_message).await.is_err() {
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
            let (sender, receiver) = async_channel::bounded::<Bytes>(1);

            executor
                .spawn(async move {
                    let mut len = Vec::new();
                    while let Ok(message) = receiver.recv().await {
                        len.clear();
                        write!(&mut len, "{}:", message.len()).unwrap();
                        writer.write_all(&len).await?;
                        writer.write_all(&message).await?;
                        writer.write_all(&[b',']).await?;
                    }

                    io::Result::Ok(())
                })
                .detach();

            sender
        };

        let inner = Arc::new(Inner {
            sender,
            internal_message_receiver,
            requests_container,
            event_handlers,
        });

        Self { inner }
    }

    pub(super) fn get_internal_message_receiver(&self) -> async_channel::Receiver<InternalMessage> {
        self.inner.internal_message_receiver.clone()
    }

    pub(crate) async fn request<R>(&self, request: R) -> Result<R::Response, RequestError>
    where
        R: Request,
    {
        // Default will work for `()` response
        let data = self
            .request_internal(request.as_method(), serde_json::to_value(request).unwrap())
            .await?
            .unwrap_or_default();
        serde_json::from_value(data).map_err(|error| RequestError::FailedToParse {
            error: error.to_string(),
        })
    }

    pub(crate) fn subscribe_to_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> SubscriptionHandler
    where
        F: Fn(Value) + Send + 'static,
    {
        self.inner.event_handlers.add(target_id, Box::new(callback))
    }

    /// Non-generic method to avoid significant duplication in final binary
    async fn request_internal(
        &self,
        method: &'static str,
        message: Value,
    ) -> Result<Option<Value>, RequestError> {
        #[derive(Debug, Serialize)]
        struct RequestMessagePrivate {
            id: u32,
            method: &'static str,
            #[serde(flatten)]
            message: Value,
        }

        let id;
        let queue_len;
        let (result_sender, result_receiver) = async_oneshot::oneshot();
        let requests_container = &self.inner.requests_container;

        {
            let mut requests_container = requests_container.lock();

            id = requests_container.next_id;
            queue_len = requests_container.handlers.len();

            requests_container.next_id = requests_container.next_id.wrapping_add(1);
            requests_container.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]: {}", method, id, message);

        let bytes = {
            let mut bytes = BytesMut::new().writer();
            serde_json::to_writer(
                &mut bytes,
                &RequestMessagePrivate {
                    id,
                    method,
                    message,
                },
            )
            .unwrap();
            bytes.into_inner()
        };

        if bytes.len() > NS_PAYLOAD_MAX_LEN {
            requests_container.lock().handlers.remove(&id);
            return Err(RequestError::MessageTooLong);
        }

        self.inner
            .sender
            .send(bytes.freeze())
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

                requests_container.lock().handlers.remove(&id);

                Err(RequestError::TimedOut)
            },
        )
        .await?;

        match result {
            Ok(data) => {
                debug!("request succeeded [method:{}, id:{}]", method, id);
                Ok(data)
            }
            Err(ResponseError { reason }) => {
                debug!("request failed [method:{}, id:{}]: {}", method, id, reason);

                Err(RequestError::Response { reason })
            }
        }
    }
}
