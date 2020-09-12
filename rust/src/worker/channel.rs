use crate::messages::Request;
use async_executor::Executor;
use async_fs::File;
use async_mutex::Mutex;
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use log::debug;
use log::trace;
use log::warn;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::error::Error;
use std::fmt::Debug;
use std::io;
use std::pin::Pin;
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;

// netstring length for a 4194304 bytes payload.
const NS_MESSAGE_MAX_LEN: usize = 4194313;
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

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

/// Subscription handler, will remove corresponding subscription when dropped
pub(crate) struct SubscriptionHandler {
    executor: Arc<Executor>,
    remove_fut: Option<Pin<Box<dyn std::future::Future<Output = ()> + Send + Sync>>>,
}

impl Drop for SubscriptionHandler {
    fn drop(&mut self) {
        self.executor
            .spawn(self.remove_fut.take().unwrap())
            .detach();
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
        // TODO: Enum?
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
        b'D' => ChannelReceiveMessage::Event(InternalMessage::Debug(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Warn log
        b'W' => ChannelReceiveMessage::Event(InternalMessage::Warn(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Error log
        b'E' => ChannelReceiveMessage::Event(InternalMessage::Error(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Dump log
        b'X' => ChannelReceiveMessage::Event(InternalMessage::Dump(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Unknown
        _ => ChannelReceiveMessage::Event(InternalMessage::Unexpected(Vec::from(bytes))),
    }
}

/// Channel is already closed
#[derive(Debug, Error)]
pub enum RequestError {
    #[error("Channel already closed")]
    ChannelClosed,
    #[error("Message is too long")]
    MessageTooLong,
    #[error("Request timed out")]
    TimedOut,
    // TODO: Enum?
    #[error("Received response error: {reason}")]
    Response { reason: String },
    #[error("Failed to parse response from worker: {error}")]
    FailedToParse {
        #[from]
        error: Box<dyn Error>,
    },
    #[error("Worker did not return any data in response")]
    NoData,
}

struct ResponseError {
    // TODO: Enum?
    reason: String,
}

type Response<T> = Result<Option<T>, ResponseError>;

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: HashMap<u32, async_oneshot::Sender<Response<Value>>>,
}

#[derive(Error, Debug)]
#[error("Handler for this event already exists, this must be an error in the library")]
pub struct HandlerAlreadyExistsError;

struct Inner {
    executor: Arc<Executor>,
    sender: async_channel::Sender<Vec<u8>>,
    internal_message_receiver: async_channel::Receiver<InternalMessage>,
    requests_container: Arc<Mutex<RequestsContainer>>,
    event_handlers: Arc<Mutex<HashMap<String, Box<dyn Fn(Value) + Send>>>>,
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

// TODO: Catch closed channel gracefully
impl Channel {
    pub(super) fn new(executor: Arc<Executor>, reader: File, mut writer: File) -> Self {
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();
        let event_handlers = Arc::<Mutex<HashMap<String, Box<dyn Fn(Value) + Send>>>>::default();

        let internal_message_receiver = {
            let requests_container = Arc::clone(&requests_container);
            let event_handlers = Arc::clone(&event_handlers);
            let (sender, receiver) = async_channel::bounded(1);

            executor
                .spawn(async move {
                    let mut len_bytes = Vec::new();
                    let mut bytes = vec![0u8; NS_PAYLOAD_MAX_LEN];
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
                                "received message payload {} is too long, max supported is {}",
                                length,
                                NS_PAYLOAD_MAX_LEN,
                            );
                        }

                        len_bytes.clear();
                        // +1 because of netstring `,` at the very end
                        reader.read_exact(&mut bytes[..(length + 1)]).await?;

                        trace!("received raw message: {:?}", String::from_utf8_lossy(&bytes[..length]));

                        match deserialize_message(&bytes[..length]) {
                            ChannelReceiveMessage::ResponseSuccess {
                                id,
                                accepted: _,
                                data,
                            } => {
                                let sender = requests_container.lock().await.handlers.remove(&id);
                                if let Some(sender) = sender {
                                    drop(sender.send(Ok(data)));
                                } else {
                                    warn!(
                                        "received success response does not match any sent request [id:{}]",
                                        id,
                                    );
                                }
                            }
                            ChannelReceiveMessage::ResponseError {
                                id,
                                error: _,
                                reason,
                            } => {
                                let sender = requests_container.lock().await.handlers.remove(&id);
                                if let Some(sender) = sender {
                                    drop(sender.send(Err(ResponseError { reason })));
                                } else {
                                    warn!(
                                        "received error response does not match any sent request [id:{}]",
                                        id,
                                    );
                                }
                            }
                            ChannelReceiveMessage::Notification(notification) => {
                                let target_id = notification
                                    .get("targetId".to_string())
                                    .map(|value| value.as_str())
                                    .flatten();
                                match target_id {
                                    Some(target_id) => {
                                        if let Some(callback) = event_handlers.lock().await.get(target_id) {
                                            callback(notification);
                                        }
                                    }
                                    None => {
                                        let unexpected_message = InternalMessage::Unexpected(Vec::from(&bytes[..length]));
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
            let (sender, receiver) = async_channel::bounded::<Vec<u8>>(1);

            executor
                .spawn(async move {
                    let mut bytes = Vec::with_capacity(NS_MESSAGE_MAX_LEN);
                    while let Ok(message) = receiver.recv().await {
                        bytes.clear();
                        bytes.extend_from_slice(message.len().to_string().as_bytes());
                        bytes.push(b':');
                        bytes.extend_from_slice(&message);
                        bytes.push(b',');

                        writer.write_all(&bytes).await?;
                    }

                    io::Result::Ok(())
                })
                .detach();

            sender
        };

        let inner = Arc::new(Inner {
            executor,
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
            error: Box::new(error),
        })
    }

    pub(crate) async fn subscribe_to_notifications<F>(
        &self,
        target_id: String,
        callback: F,
    ) -> Result<SubscriptionHandler, HandlerAlreadyExistsError>
    where
        F: Fn(Value) + Send + 'static,
    {
        {
            let mut event_handlers = self.inner.event_handlers.lock().await;
            if event_handlers.contains_key(&target_id) {
                return Err(HandlerAlreadyExistsError {});
            }

            event_handlers.insert(target_id.clone(), Box::new(callback));
        }

        let event_handlers = self.inner.event_handlers.clone();
        Ok(SubscriptionHandler {
            executor: Arc::clone(&self.inner.executor),
            remove_fut: Some(Box::pin(async move {
                event_handlers.lock().await.remove(&target_id);
            })),
        })
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
            let mut requests_container = requests_container.lock().await;

            id = requests_container.next_id;
            queue_len = requests_container.handlers.len();

            requests_container.next_id = requests_container.next_id.wrapping_add(1);
            requests_container.handlers.insert(id, result_sender);
        }

        debug!("request() [method:{}, id:{}]", method, id);

        let serialized_message = serde_json::to_vec(&RequestMessagePrivate {
            id,
            method,
            message,
        })
        .unwrap();

        if serialized_message.len() > NS_PAYLOAD_MAX_LEN {
            requests_container.lock().await.handlers.remove(&id);
            return Err(RequestError::MessageTooLong);
        }

        self.inner
            .sender
            .send(serialized_message)
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

                requests_container.lock().await.handlers.remove(&id);

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
