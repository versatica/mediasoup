// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use async_channel::{Receiver, Sender};
use async_executor::Executor;
use async_fs::File as AsyncFile;
use async_process::unix::CommandExt;
use async_process::{Child, Command};
use futures_lite::{future, AsyncWriteExt};
use nix::unistd;
use std::fs::File as StdFile;
use std::os::unix::io::{FromRawFd, RawFd};
use std::{io, thread};

// netstring length for a 4194304 bytes payload.
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

#[derive(Debug)]
pub enum ChannelMessage {
    /// JSON message
    Json(String),
    /// Debug log
    Debug(String),
    /// Warn log
    Warn(String),
    /// Error log
    Error(String),
    /// Dump log
    Dump(String),
    /// Unknown
    Unknown { data: Vec<u8> },
}

fn deserialize_message(bytes: &[u8]) -> ChannelMessage {
    // TODO: More specific message parsing
    match bytes[0] {
        // JSON message
        b'{' => ChannelMessage::Json(unsafe { String::from_utf8_unchecked(Vec::from(bytes)) }),
        // Debug log
        b'D' => {
            ChannelMessage::Debug(unsafe { String::from_utf8_unchecked(Vec::from(&bytes[1..])) })
        }
        // Warn log
        b'W' => {
            ChannelMessage::Warn(unsafe { String::from_utf8_unchecked(Vec::from(&bytes[1..])) })
        }
        // Error log
        b'E' => {
            ChannelMessage::Error(unsafe { String::from_utf8_unchecked(Vec::from(&bytes[1..])) })
        }
        // Dump log
        b'X' => {
            ChannelMessage::Dump(unsafe { String::from_utf8_unchecked(Vec::from(&bytes[1..])) })
        }
        // Unknown
        _ => ChannelMessage::Unknown {
            data: Vec::from(bytes),
        },
    }
}

fn create_channel_pair(
    read_channel_name: String,
    executor: &Executor,
    reader: StdFile,
    mut writer: AsyncFile,
) -> (Sender<Vec<u8>>, Receiver<ChannelMessage>) {
    let receiver = {
        use std::io::{BufRead, BufReader, Read};

        let (sender, receiver) = async_channel::bounded(1);

        // TODO: We'd want to have it async here, but https://github.com/stjepang/async-fs/issues/4
        thread::Builder::new()
            .name(read_channel_name)
            .spawn(move || {
                let mut len_bytes = Vec::new();
                let mut bytes = vec![0u8; NS_PAYLOAD_MAX_LEN];
                let mut reader = BufReader::new(reader);

                loop {
                    let read_bytes = reader.read_until(b':', &mut len_bytes)?;
                    if read_bytes == 0 {
                        // EOF
                        break;
                    }
                    let length = String::from_utf8_lossy(&len_bytes[..(read_bytes - 1)])
                        .parse::<usize>()
                        .unwrap();
                    len_bytes.clear();
                    // +1 because of netstring's `,` at the very end
                    reader.read_exact(&mut bytes[..(length + 1)])?;
                    let message = deserialize_message(&bytes[..length]);
                    if future::block_on(sender.send(message)).is_err() {
                        break;
                    }
                }

                io::Result::Ok(())
            })
            .unwrap();

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
    pub channel: (Sender<Vec<u8>>, Receiver<ChannelMessage>),
    pub payload_channel: (Sender<Vec<u8>>, Receiver<ChannelMessage>),
}

fn create_pipe() -> (RawFd, RawFd) {
    let (read, write) = unistd::pipe().unwrap();
    // nix::fcntl::fcntl(
    //     read,
    //     nix::fcntl::FcntlArg::F_SETFD(nix::fcntl::FdFlag::FD_CLOEXEC),
    // )
    // .unwrap();
    // nix::fcntl::fcntl(
    //     write,
    //     nix::fcntl::FcntlArg::F_SETFD(nix::fcntl::FdFlag::FD_CLOEXEC),
    // )
    // .unwrap();

    (read, write)
}

pub fn spawn_with_worker_channels(
    executor: &Executor,
    command: &mut Command,
) -> io::Result<SpawnResult> {
    let (producer_fd_read, producer_fd_write) = create_pipe();
    let (consumer_fd_read, consumer_fd_write) = create_pipe();
    let (producer_payload_fd_read, producer_payload_fd_write) = create_pipe();
    let (consumer_payload_fd_read, consumer_payload_fd_write) = create_pipe();

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

    let pid = child.id();

    Ok(SpawnResult {
        child,
        channel: create_channel_pair(
            format!("consumer_file {}", pid),
            &executor,
            consumer_file,
            producer_file,
        ),
        payload_channel: create_channel_pair(
            format!("consumer_payload_file {}", pid),
            &executor,
            consumer_payload_file,
            producer_payload_file,
        ),
    })
}
