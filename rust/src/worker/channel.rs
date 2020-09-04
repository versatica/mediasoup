use crate::data_structures::{ChannelReceiveMessage, JsonReceiveMessage, RequestMessage};
use async_channel::SendError;
use async_executor::Executor;
use async_fs::File;
use async_mutex::Mutex;
use futures_lite::io::BufReader;
use futures_lite::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use log::debug;
use log::warn;
use serde::Serialize;
use std::collections::HashMap;
use std::io;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;
use thiserror::Error;

// netstring length for a 4194304 bytes payload.
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

fn deserialize_message(bytes: &[u8]) -> ChannelReceiveMessage {
    match bytes[0] {
        // JSON message
        b'{' => {
            let contents = serde_json::from_slice(bytes).unwrap();
            ChannelReceiveMessage::Json(contents)
        }
        // Debug log
        b'D' => ChannelReceiveMessage::Debug(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        }),
        // Warn log
        b'W' => ChannelReceiveMessage::Warn(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        }),
        // Error log
        b'E' => ChannelReceiveMessage::Error(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        }),
        // Dump log
        b'X' => ChannelReceiveMessage::Dump(unsafe {
            String::from_utf8_unchecked(Vec::from(&bytes[1..]))
        }),
        // Unknown
        _ => ChannelReceiveMessage::Unexpected {
            data: Vec::from(bytes),
        },
    }
}

/// Channel is already closed
#[derive(Error, Debug)]
#[error("Channel already closed")]
pub struct ChannelClosedError;

pub struct ResponseError {
    reason: String,
}

type Response = Result<Option<()>, ResponseError>;

#[derive(Default)]
struct RequestsContainer {
    next_id: u32,
    handlers: HashMap<u32, async_oneshot::Sender<Response>>,
}

// TODO: Close sender/receiver when Channel is dropped
pub struct Channel {
    sender: async_channel::Sender<Vec<u8>>,
    receiver: async_channel::Receiver<ChannelReceiveMessage>,
    requests_container: Arc<Mutex<RequestsContainer>>,
}

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
                        len_bytes.clear();
                        // +1 because of netstring's `,` at the very end
                        reader.read_exact(&mut bytes[..(length + 1)]).await?;
                        let message = deserialize_message(&bytes[..length]);
                        match message {
                            ChannelReceiveMessage::Json(message) => match message {
                                JsonReceiveMessage::Success {
                                    id,
                                    accepted: _,
                                    data,
                                } => {
                                    let sender = requests_container.lock().await.handlers.remove(&id);
                                    if let Some(sender) = sender {
                                        drop(sender.send(Ok(data)));
                                    } else {
                                        warn!("received success response does not match any sent request [id:{}]", id);
                                    }
                                }
                                JsonReceiveMessage::Error {
                                    id,
                                    error: _,
                                    reason,
                                } => {
                                    let sender = requests_container.lock().await.handlers.remove(&id);
                                    if let Some(sender) = sender {
                                        drop(sender.send(Err(ResponseError {
                                            reason
                                        })));
                                    } else {
                                        warn!("received error response does not match any sent request [id:{}]", id);
                                    }
                                }
                                message => {
                                    // TODO: This type should be more limited
                                    if sender.send(ChannelReceiveMessage::Json(message)).await.is_err() {
                                        break;
                                    }

                                }
                            },
                            message => {
                                // TODO: This type should be more limited
                                if sender.send(message).await.is_err() {
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
                    let mut bytes = Vec::with_capacity(NS_PAYLOAD_MAX_LEN);
                    // TODO: Stringify messages here and received non-stringified messages over the
                    //  channel
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

        Self {
            sender,
            receiver,
            requests_container,
        }
    }

    pub fn get_receiver(&self) -> async_channel::Receiver<ChannelReceiveMessage> {
        self.receiver.clone()
    }

    pub async fn request(&self, message: RequestMessage) -> Result<Response, ChannelClosedError> {
        #[derive(Debug, Serialize)]
        struct RequestMessagePrivate {
            id: u32,
            method: &'static str,
            #[serde(flatten)]
            message: RequestMessage,
        }

        let id;
        let method = message.as_method();
        let (result_sender, result_receiver) = async_oneshot::oneshot::<Response>();

        {
            let mut requests_container = self.requests_container.lock().await;

            id = requests_container.next_id;

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

        self.sender
            .send(serialized_message)
            .await
            .map_err(|_| ChannelClosedError {})?;

        result_receiver.await.map_err(|_| ChannelClosedError {})
    }
}
