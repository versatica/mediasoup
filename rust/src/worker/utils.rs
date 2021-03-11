// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use crate::worker::{Channel, PayloadChannel};
use async_executor::Executor;
use async_fs::File;
use nix::unistd;
use once_cell::sync::Lazy;
use parking_lot::Mutex;
use std::io;
use std::os::unix::io::FromRawFd;
use std::sync::Arc;

static SPAWNING: Lazy<Mutex<()>> = Lazy::new(|| Mutex::new(()));

pub(super) struct SpawnResult {
    pub(super) channel: Channel,
    pub(super) payload_channel: PayloadChannel,
}

// TODO: Returns result even though never fails
pub(super) fn spawn_with_worker_channels(
    executor: Arc<Executor<'static>>,
    args: Vec<String>,
) -> io::Result<SpawnResult> {
    // Take a lock to make sure we don't spawn workers from multiple threads concurrently, this
    // causes racy issues
    let _lock = SPAWNING.lock();
    let (producer_fd_read, producer_fd_write) = unistd::pipe().unwrap();
    let (consumer_fd_read, consumer_fd_write) = unistd::pipe().unwrap();
    let (producer_payload_fd_read, producer_payload_fd_write) = unistd::pipe().unwrap();
    let (consumer_payload_fd_read, consumer_payload_fd_write) = unistd::pipe().unwrap();

    // TODO: This is currently unstoppable thread
    std::thread::spawn(move || {
        let result = mediasoup_sys::run(
            args,
            producer_fd_read,
            consumer_fd_write,
            producer_payload_fd_read,
            consumer_payload_fd_write,
        );

        println!("Worker exit result: {:?}", result);
    });

    let producer_file = unsafe { File::from_raw_fd(producer_fd_write) };
    let consumer_file = unsafe { File::from_raw_fd(consumer_fd_read) };
    let producer_payload_file = unsafe { File::from_raw_fd(producer_payload_fd_write) };
    let consumer_payload_file = unsafe { File::from_raw_fd(consumer_payload_fd_read) };

    Ok(SpawnResult {
        channel: Channel::new(Arc::clone(&executor), consumer_file, producer_file),
        payload_channel: PayloadChannel::new(
            executor,
            consumer_payload_file,
            producer_payload_file,
        ),
    })
}
