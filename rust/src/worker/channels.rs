// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use async_channel::{Receiver, Sender};
use async_executor::Executor;
use async_fs::File as AsyncFile;
use async_process::unix::CommandExt;
use async_process::{Child, Command};
use futures_lite::io::BufReader;
use futures_lite::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use nix::unistd;
use std::fs::File as StdFile;
use std::io;
use std::os::unix::io::{FromRawFd, RawFd};

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
    Unknown { command: u8, data: Vec<u8> },
}

fn deserialize_message(command: u8, data: Vec<u8>) -> ChannelMessage {
    match command {
        // JSON message
        b'{' => ChannelMessage::Json(unsafe { String::from_utf8_unchecked(data) }),
        // Debug log
        b'D' => ChannelMessage::Debug(unsafe { String::from_utf8_unchecked(data) }),
        // Warn log
        b'W' => ChannelMessage::Warn(unsafe { String::from_utf8_unchecked(data) }),
        // Error log
        b'E' => ChannelMessage::Error(unsafe { String::from_utf8_unchecked(data) }),
        // Dump log
        b'X' => ChannelMessage::Dump(unsafe { String::from_utf8_unchecked(data) }),
        // Unknown
        _ => ChannelMessage::Unknown { command, data },
    }
}

fn create_channel_pair(
    executor: &Executor,
    reader: AsyncFile,
    mut writer: AsyncFile,
) -> (Sender<Vec<u8>>, Receiver<ChannelMessage>) {
    let receiver = {
        let (sender, receiver) = async_channel::bounded(1);

        executor
            .spawn(async move {
                let mut bytes = vec![0u8; NS_PAYLOAD_MAX_LEN];
                let mut reader = BufReader::new(reader);

                loop {
                    let read_bytes = reader.read_until(b':', &mut bytes).await?;
                    bytes.pop();
                    let length = String::from_utf8_lossy(&bytes[..read_bytes])
                        .parse::<usize>()
                        .unwrap();
                    // +1 because of netstring's `,` at the very end
                    reader.read_exact(&mut bytes[..(length + 1)]).await?;
                    // TODO: Parse messages here and send parsed messages over the channel
                    let message = deserialize_message(bytes[0], Vec::from(&bytes[1..length]));
                    println!("Received");
                    let _ = sender.send(message);
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
    pub channel: (Sender<Vec<u8>>, Receiver<ChannelMessage>),
    pub payload_channel: (Sender<Vec<u8>>, Receiver<ChannelMessage>),
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
            // Duplicate such that it creates new file descriptors in child channel
            let producer_fd_read_tmp = unistd::dup(producer_fd_read).unwrap();
            let consumer_fd_write_tmp = unistd::dup(consumer_fd_write).unwrap();
            let producer_payload_fd_read_tmp = unistd::dup(producer_payload_fd_read).unwrap();
            let consumer_payload_fd_write_tmp = unistd::dup(consumer_payload_fd_write).unwrap();
            // Unused in child
            unistd::close(producer_fd_write).unwrap();
            unistd::close(consumer_fd_read).unwrap();
            unistd::close(producer_payload_fd_write).unwrap();
            unistd::close(consumer_payload_fd_read).unwrap();
            // Duplicated above
            unistd::close(producer_fd_read).unwrap();
            unistd::close(consumer_fd_write).unwrap();
            unistd::close(producer_payload_fd_read).unwrap();
            unistd::close(consumer_payload_fd_write).unwrap();
            // Now duplicate into file descriptor indexes we actually want
            unistd::dup2(producer_fd_read_tmp, 3).unwrap();
            unistd::dup2(consumer_fd_write_tmp, 4).unwrap();
            unistd::dup2(producer_payload_fd_read_tmp, 5).unwrap();
            unistd::dup2(consumer_payload_fd_write_tmp, 6).unwrap();
            // Duplicated above
            unistd::close(producer_fd_read_tmp).unwrap();
            unistd::close(consumer_fd_write_tmp).unwrap();
            unistd::close(producer_payload_fd_read_tmp).unwrap();
            unistd::close(consumer_payload_fd_write_tmp).unwrap();

            Ok(())
        });
    };

    let child = command.spawn()?;

    let producer_file: AsyncFile;
    let consumer_file: AsyncFile;
    let producer_payload_file: AsyncFile;
    let consumer_payload_file: AsyncFile;
    // Unused in parent
    unistd::close(producer_fd_read).expect("Failed to close fd");
    unistd::close(consumer_fd_write).expect("Failed to close fd");
    unistd::close(producer_payload_fd_read).expect("Failed to close fd");
    unistd::close(consumer_payload_fd_write).expect("Failed to close fd");

    producer_file = unsafe { StdFile::from_raw_fd(producer_fd_write) }.into();
    consumer_file = unsafe { StdFile::from_raw_fd(consumer_fd_read) }.into();
    producer_payload_file = unsafe { StdFile::from_raw_fd(producer_payload_fd_write) }.into();
    consumer_payload_file = unsafe { StdFile::from_raw_fd(consumer_payload_fd_read) }.into();

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
