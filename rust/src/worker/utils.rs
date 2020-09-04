// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use async_channel::{Receiver, Sender};
use async_executor::Executor;
use async_fs::File as AsyncFile;
use async_process::unix::CommandExt;
use async_process::{Child, Command};
use futures_lite::io::BufReader;
use futures_lite::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use nix::unistd;
use serde::Deserialize;
use std::fs::File as StdFile;
use std::io;
use std::os::unix::io::FromRawFd;

// netstring length for a 4194304 bytes payload.
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum NotificationEvent {
    Running,
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
pub enum JsonReceiveMessage {
    #[serde(rename_all = "camelCase")]
    Notification {
        target_id: String,
        event: NotificationEvent,
        data: Option<()>,
    },
    MgsAccepted {
        id: u32,
        accepted: bool,
    },
    MgsError {
        id: u32,
        error: bool,
        reason: String,
    },
}

#[derive(Debug)]
pub enum ChannelReceiveMessage {
    /// JSON message
    Json(JsonReceiveMessage),
    /// Debug log
    Debug(String),
    /// Warn log
    Warn(String),
    /// Error log
    Error(String),
    /// Dump log
    Dump(String),
    /// Unknown
    Unexpected { data: Vec<u8> },
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

fn create_channel_pair(
    executor: &Executor,
    reader: AsyncFile,
    mut writer: AsyncFile,
) -> (Sender<Vec<u8>>, Receiver<ChannelReceiveMessage>) {
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

    (sender, receiver)
}

pub struct SpawnResult {
    pub child: Child,
    // TODO: Wrapper types (probably with Deref and DerefMut impl) for channels to close threads and
    //  tasks when channel is dropped
    pub channel: (Sender<Vec<u8>>, Receiver<ChannelReceiveMessage>),
    pub payload_channel: (Sender<Vec<u8>>, Receiver<ChannelReceiveMessage>),
}

pub fn spawn_with_worker_channels(
    executor: &Executor,
    command: &mut Command,
) -> io::Result<SpawnResult> {
    let (producer_fd_read, producer_fd_write) = unistd::pipe().unwrap();
    let (consumer_fd_read, consumer_fd_write) = unistd::pipe().unwrap();
    let (producer_payload_fd_read, producer_payload_fd_write) = unistd::pipe().unwrap();
    let (consumer_payload_fd_read, consumer_payload_fd_write) = unistd::pipe().unwrap();

    unsafe {
        command.pre_exec(move || {
            // Unused in child
            unistd::close(producer_fd_write).unwrap();
            unistd::close(consumer_fd_read).unwrap();
            unistd::close(producer_payload_fd_write).unwrap();
            unistd::close(consumer_payload_fd_read).unwrap();
            // Now duplicate into file descriptor indexes we need
            if producer_fd_read != 3 {
                unistd::dup2(producer_fd_read, 3).unwrap();
                unistd::close(producer_fd_read).unwrap();
            }
            if consumer_fd_write != 4 {
                unistd::dup2(consumer_fd_write, 4).unwrap();
                unistd::close(consumer_fd_write).unwrap();
            }
            if producer_payload_fd_read != 5 {
                unistd::dup2(producer_payload_fd_read, 5).unwrap();
                unistd::close(producer_payload_fd_read).unwrap();
            }
            if consumer_payload_fd_write != 6 {
                unistd::dup2(consumer_payload_fd_write, 6).unwrap();
                unistd::close(consumer_payload_fd_write).unwrap();
            }

            Ok(())
        });
    };

    let producer_file = unsafe { StdFile::from_raw_fd(producer_fd_write) }.into();
    let consumer_file = unsafe { StdFile::from_raw_fd(consumer_fd_read) }.into();
    let producer_payload_file = unsafe { StdFile::from_raw_fd(producer_payload_fd_write) }.into();
    let consumer_payload_file = unsafe { StdFile::from_raw_fd(consumer_payload_fd_read) }.into();

    let child = command.spawn()?;

    // Unused in parent
    unistd::close(producer_fd_read).expect("Failed to close fd");
    unistd::close(consumer_fd_write).expect("Failed to close fd");
    unistd::close(producer_payload_fd_read).expect("Failed to close fd");
    unistd::close(consumer_payload_fd_write).expect("Failed to close fd");

    Ok(SpawnResult {
        child,
        channel: create_channel_pair(&executor, consumer_file, producer_file),
        payload_channel: create_channel_pair(
            &executor,
            consumer_payload_file,
            producer_payload_file,
        ),
    })
}
