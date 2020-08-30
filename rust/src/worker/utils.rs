// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use async_channel::{Receiver, Sender};
use async_executor::Executor;
use async_fs::File as AsyncFile;
use async_process::unix::CommandExt;
use async_process::Command;
use futures_lite::io::BufReader;
use futures_lite::{AsyncBufReadExt, AsyncReadExt, AsyncWriteExt};
use std::error::Error;
use std::fs::File as StdFile;
use std::io;
use std::os::raw::c_int;
use std::os::unix::io::FromRawFd;

// netstring length for a 4194304 bytes payload.
const NS_PAYLOAD_MAX_LEN: usize = 4194304;

pub struct WorkerChannel {
    pub receiver: Receiver<Vec<u8>>,
    pub sender: Sender<Vec<u8>>,
}

impl WorkerChannel {
    fn new(executor: &Executor, reader: AsyncFile, mut writer: AsyncFile) -> Self {
        let receiver = {
            let (sender, receiver) = async_channel::bounded::<Vec<u8>>(1);

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
                        println!("Received");
                        let _ = sender.send(Vec::from(&bytes[..length]));
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
                    // channel
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
}

pub struct WorkerChannels {
    pub channel: WorkerChannel,
    pub payload_channel: WorkerChannel,
}

pub fn setup_worker_channels(executor: &Executor, command: &mut Command) -> WorkerChannels {
    const READ_END: usize = 0;
    const WRITE_END: usize = 1;

    let mut producer_fds: [c_int; 2] = [0; 2];
    let mut consumer_fds: [c_int; 2] = [0; 2];
    let mut producer_payload_fds: [c_int; 2] = [0; 2];
    let mut consumer_payload_fds: [c_int; 2] = [0; 2];

    unsafe {
        libc::pipe(producer_fds.as_mut_ptr());
        libc::pipe(consumer_fds.as_mut_ptr());
        libc::pipe(producer_payload_fds.as_mut_ptr());
        libc::pipe(consumer_payload_fds.as_mut_ptr());
    }

    unsafe {
        command.pre_exec(move || {
            libc::dup2(producer_fds[READ_END], 3);
            libc::dup2(consumer_fds[WRITE_END], 4);
            libc::dup2(producer_payload_fds[READ_END], 5);
            libc::dup2(consumer_payload_fds[WRITE_END], 6);

            Ok(())
        });
    };

    let producer_file: AsyncFile;
    let consumer_file: AsyncFile;
    let producer_payload_file: AsyncFile;
    let consumer_payload_file: AsyncFile;
    unsafe {
        libc::close(producer_fds[READ_END]);
        libc::close(consumer_fds[WRITE_END]);
        libc::close(producer_payload_fds[READ_END]);
        libc::close(consumer_payload_fds[WRITE_END]);
        producer_file = StdFile::from_raw_fd(producer_fds[WRITE_END]).into();
        consumer_file = StdFile::from_raw_fd(consumer_fds[READ_END]).into();
        producer_payload_file = StdFile::from_raw_fd(producer_payload_fds[WRITE_END]).into();
        consumer_payload_file = StdFile::from_raw_fd(consumer_payload_fds[READ_END]).into();
    }

    WorkerChannels {
        channel: WorkerChannel::new(&executor, consumer_file, producer_file),
        payload_channel: WorkerChannel::new(
            &executor,
            consumer_payload_file,
            producer_payload_file,
        ),
    }
}
