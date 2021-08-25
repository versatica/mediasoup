use crate::messages::Request;
use crate::worker::common::{EventHandlers, SubscriptionTarget, WeakEventHandlers};
use crate::worker::{RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_fs::File;
use bytes::Bytes;
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncReadExt, AsyncWriteExt};
use log::{debug, trace, warn};
use lru::LruCache;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt::Debug;
use std::io;
use std::sync::{Arc, Weak};
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

pub(crate) struct BufferMessagesGuard {
    target_id: SubscriptionTarget,
    buffered_notifications_for: Arc<Mutex<HashMap<SubscriptionTarget, Vec<Value>>>>,
    event_handlers_weak: WeakEventHandlers<Value>,
}

impl Drop for BufferMessagesGuard {
    fn drop(&mut self) {
        let mut buffered_notifications_for = self.buffered_notifications_for.lock();
        if let Some(notifications) = buffered_notifications_for.remove(&self.target_id) {
            if let Some(event_handlers) = self.event_handlers_weak.upgrade() {
                for notification in notifications {
                    event_handlers.call_callbacks_with_value(&self.target_id, notification);
                }
            }
        }
    }
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
    requests_container_weak: Weak<Mutex<RequestsContainer>>,
    buffered_notifications_for: Arc<Mutex<HashMap<SubscriptionTarget, Vec<Value>>>>,
    event_handlers_weak: WeakEventHandlers<Value>,
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
    pub(super) fn new(executor: &Executor<'static>, reader: File, mut writer: File) -> Self {
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let requests_container_weak = Arc::downgrade(&requests_container);
        let buffered_notifications_for =
            Arc::<Mutex<HashMap<SubscriptionTarget, Vec<Value>>>>::default();
        let event_handlers = EventHandlers::new();
        let event_handlers_weak = event_handlers.downgrade();

        let internal_message_receiver = {
            let buffered_notifications_for = buffered_notifications_for.clone();
            let (sender, receiver) = async_channel::bounded(1);

            executor
                .spawn(async move {
                    let mut len_bytes = [0u8; 4];
                    let mut bytes = Vec::new();
                    let mut reader = BufReader::new(reader);
                    // This this contain cache of targets that are known to not have buffering, so
                    // that we can avoid Mutex locking overhead for them
                    let mut non_buffered_notifications =
                        LruCache::<SubscriptionTarget, ()>::new(1000);

                    loop {
                        reader.read_exact(&mut len_bytes).await?;
                        let length = u32::from_ne_bytes(len_bytes) as usize;

                        if length > NS_PAYLOAD_MAX_LEN {
                            warn!(
                                "received message {} is too long, max supported is {}",
                                length, NS_PAYLOAD_MAX_LEN,
                            );
                        }

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
                            ChannelReceiveMessage::ResponseSuccess {
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
                            ChannelReceiveMessage::ResponseError {
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
                            ChannelReceiveMessage::Notification(notification) => {
                                let target_id = notification.get("targetId").and_then(|value| {
                                    let str = value.as_str()?;
                                    str.parse().ok().map(SubscriptionTarget::Uuid).or_else(|| {
                                        str.parse().ok().map(SubscriptionTarget::Number)
                                    })
                                });

                                if let Some(target_id) = target_id {
                                    if !non_buffered_notifications.contains(&target_id) {
                                        let mut buffer_notifications_for =
                                            buffered_notifications_for.lock();
                                        // Check if we need to buffer notifications for this
                                        // target_id
                                        if let Some(list) =
                                            buffer_notifications_for.get_mut(&target_id)
                                        {
                                            list.push(notification);
                                            continue;
                                        }

                                        // Remember we don't need to buffer these
                                        non_buffered_notifications.put(target_id, ());
                                    }
                                    event_handlers
                                        .call_callbacks_with_value(&target_id, notification);
                                } else {
                                    let unexpected_message =
                                        InternalMessage::Unexpected(Vec::from(&bytes[..length]));
                                    if sender.send(unexpected_message).await.is_err() {
                                        break;
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
                    while let Ok(message) = receiver.recv().await {
                        writer
                            .write_all(&(message.len() as u32).to_ne_bytes())
                            .await?;
                        writer.write_all(&message).await?;
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
            buffered_notifications_for,
            event_handlers_weak,
        });

        Self { inner }
    }

    pub(super) fn get_internal_message_receiver(&self) -> async_channel::Receiver<InternalMessage> {
        self.inner.internal_message_receiver.clone()
    }

    /// This allows to enable buffering for messages for specific target while the target itself is
    /// being created. This allows to avoid missing notifications due to race conditions.
    pub(crate) fn buffer_messages_for(&self, target_id: SubscriptionTarget) -> BufferMessagesGuard {
        let buffered_notifications_for = Arc::clone(&self.inner.buffered_notifications_for);
        let event_handlers_weak = self.inner.event_handlers_weak.clone();
        buffered_notifications_for
            .lock()
            .entry(target_id)
            .or_default();
        BufferMessagesGuard {
            target_id,
            buffered_notifications_for,
            event_handlers_weak,
        }
    }

    pub(crate) async fn request<R>(&self, request: R) -> Result<R::Response, RequestError>
    where
        R: Request,
    {
        let method = request.as_method();

        #[derive(Debug, Serialize)]
        struct RequestMessagePrivate<'a, R: Serialize> {
            id: u32,
            method: &'static str,
            #[serde(flatten)]
            request: &'a R,
        }

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

        let bytes = Bytes::from(
            serde_json::to_vec(&RequestMessagePrivate {
                id,
                method,
                request: &request,
            })
            .unwrap(),
        );

        if bytes.len() > NS_PAYLOAD_MAX_LEN {
            self.inner
                .requests_container_weak
                .upgrade()
                .ok_or(RequestError::ChannelClosed)?
                .lock()
                .handlers
                .remove(&id);
            return Err(RequestError::MessageTooLong);
        }

        self.inner
            .sender
            .send(bytes)
            .await
            .map_err(|_| RequestError::ChannelClosed {})?;

        let result = future::or(
            async {
                result_receiver
                    .await
                    .map_err(|_| RequestError::ChannelClosed {})
            },
            async {
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

    pub(crate) fn subscribe_to_notifications<F>(
        &self,
        target_id: SubscriptionTarget,
        callback: F,
    ) -> Option<SubscriptionHandler>
    where
        F: Fn(Value) + Send + Sync + 'static,
    {
        self.inner
            .event_handlers_weak
            .upgrade()
            .map(|event_handlers| event_handlers.add(target_id, Box::new(callback)))
    }
}
