use crate::data_structures::{ChannelReceiveMessage, RequestMessage, WorkerLogLevel, WorkerLogTag};
use async_channel::{Receiver, Sender};
use async_executor::Executor;
use async_fs::File;
use futures_lite::io::BufReader;
use futures_lite::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use serde::{Deserialize, Serialize};
use std::io;

// netstring length for a 4194304 bytes payload.
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

#[derive(Debug, Serialize)]
struct RequestMessagePrivate {
    id: u32,
    #[serde(flatten)]
    message: RequestMessage,
}

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

// TODO: Close sender/receiver when Channel is dropped
pub struct Channel {
    receiver: Receiver<ChannelReceiveMessage>,
    sender: Sender<Vec<u8>>,
}

impl Channel {
    pub fn new(executor: &Executor, reader: File, mut writer: File) -> Self {
        let receiver = {
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
                        if sender.send(message).await.is_err() {
                            break;
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

        Self { receiver, sender }
    }

    pub fn get_receiver(&self) -> Receiver<ChannelReceiveMessage> {
        self.receiver.clone()
    }

    pub fn get_sender(&self) -> Sender<Vec<u8>> {
        self.sender.clone()
    }
}
