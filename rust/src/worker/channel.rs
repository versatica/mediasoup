use crate::data_structures::RequestMessage;
use async_executor::Executor;
use async_fs::File;
use async_mutex::Mutex;
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use log::debug;
use log::warn;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::io;
use std::sync::Arc;
use std::time::Duration;
use thiserror::Error;

// netstring length for a 4194304 bytes payload.
const NS_MESSAGE_MAX_LEN: usize = 4194313;
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum NotificationEvent {
    Running,
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum EventMessage {
    #[serde(rename_all = "camelCase")]
    Notification {
        target_id: String,
        event: NotificationEvent,
        // TODO: Can there be data here?
        data: Option<()>,
    },
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
        error: bool,
        // TODO: Enum?
        reason: String,
    },
    Event(EventMessage),
}

fn deserialize_message(bytes: &[u8]) -> ChannelReceiveMessage {
    match bytes[0] {
        // JSON message
        b'{' => serde_json::from_slice(bytes).unwrap(),
        // Debug log
        b'D' => ChannelReceiveMessage::Event(EventMessage::Debug(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Warn log
        b'W' => ChannelReceiveMessage::Event(EventMessage::Warn(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Error log
        b'E' => ChannelReceiveMessage::Event(EventMessage::Error(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Dump log
        b'X' => ChannelReceiveMessage::Event(EventMessage::Dump(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        })),
        // Unknown
        _ => ChannelReceiveMessage::Event(EventMessage::Unexpected(Vec::from(bytes))),
    }
}

/// Channel is already closed
#[derive(Error, Debug)]
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
    FailedToParse { error: serde_json::Error },
    #[error("Worker did not return any data in response")]
    NoData,
}

pub struct ResponseError {
    // TODO: Enum?
    reason: String,
}

type Response<T> = Result<Option<T>, ResponseError>;

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: HashMap<u32, async_oneshot::Sender<Response<Value>>>,
}

struct Inner {
    sender: async_channel::Sender<Vec<u8>>,
    receiver: async_channel::Receiver<EventMessage>,
    requests_container: Arc<Mutex<RequestsContainer>>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        self.sender.close();
        self.receiver.close();
    }
}

#[derive(Clone)]
pub struct Channel {
    inner: Arc<Inner>,
}

// TODO: Catch closed channel gracefully
impl Channel {
    pub fn new(executor: &Executor, reader: File, mut writer: File) -> Self {
        let requests_container = Arc::<Mutex<RequestsContainer>>::default();

        let receiver = {
            let requests_container = Arc::clone(&requests_container);
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
                        // +1 because of netstring's `,` at the very end
                        reader.read_exact(&mut bytes[..(length + 1)]).await?;

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
            sender,
            receiver,
            requests_container,
        });

        Self { inner }
    }

    pub fn get_receiver(&self) -> async_channel::Receiver<EventMessage> {
        self.inner.receiver.clone()
    }

    pub async fn request<T>(&self, message: RequestMessage) -> Result<T, RequestError>
    where
        T: DeserializeOwned,
    {
        // Default will work for `()` response
        let data = self.request_internal(message).await?.unwrap_or_default();
        serde_json::from_value(data).map_err(|error| RequestError::FailedToParse { error })
    }

    /// Non-generic method to avoid significant duplication in final binary
    async fn request_internal(
        &self,
        message: RequestMessage,
    ) -> Result<Option<Value>, RequestError> {
        #[derive(Debug, Serialize)]
        struct RequestMessagePrivate {
            id: u32,
            method: &'static str,
            #[serde(flatten)]
            message: RequestMessage,
        }

        let id;
        let queue_len;
        let method = message.as_method();
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

        let result = future::race(
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
