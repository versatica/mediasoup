// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use crate::worker::channel::BufferMessagesGuard;
use crate::worker::{Channel, PayloadChannel, WorkerId};
use async_executor::Executor;
use async_fs::File;
use async_oneshot::Receiver;
use std::ffi::CString;
use std::mem;
use std::os::raw::{c_char, c_int};
use std::os::unix::io::FromRawFd;
use std::sync::Arc;
use thiserror::Error;

/// Worker exit error
#[derive(Debug, Copy, Clone, Error)]
pub enum ExitError {
    /// Generic error.
    #[error("Worker exited with generic error")]
    Generic,
    /// Settings error.
    #[error("Worker exited with settings error")]
    Settings,
    /// Unknown error.
    #[error("Worker exited with unknown error and status code {status_code}")]
    Unknown {
        /// Status code returned by worker
        status_code: i32,
    },
    /// Unexpected error.
    #[error("Worker exited unexpectedly")]
    Unexpected,
}

fn pipe() -> [c_int; 2] {
    unsafe {
        let mut fds = mem::MaybeUninit::<[c_int; 2]>::uninit();

        if libc::pipe(fds.as_mut_ptr().cast::<c_int>()) != 0 {
            panic!(
                "libc::pipe() failed with code {}",
                *libc::__errno_location()
            );
        }

        fds.assume_init()
    }
}

pub(super) struct WorkerRunResult {
    pub(super) channel: Channel,
    pub(super) payload_channel: PayloadChannel,
    pub(super) status_receiver: Receiver<Result<(), ExitError>>,
    pub(super) buffer_worker_messages_guard: BufferMessagesGuard,
}

pub(super) fn run_worker_with_channels(
    id: WorkerId,
    executor: Arc<Executor<'static>>,
    args: Vec<String>,
) -> WorkerRunResult {
    let [producer_fd_read, producer_fd_write] = pipe();
    let [consumer_fd_read, consumer_fd_write] = pipe();
    let [producer_payload_fd_read, producer_payload_fd_write] = pipe();
    let [consumer_payload_fd_read, consumer_payload_fd_write] = pipe();
    let (mut status_sender, status_receiver) = async_oneshot::oneshot();

    let producer_file = unsafe { File::from_raw_fd(producer_fd_write) };
    let consumer_file = unsafe { File::from_raw_fd(consumer_fd_read) };
    let producer_payload_file = unsafe { File::from_raw_fd(producer_payload_fd_write) };
    let consumer_payload_file = unsafe { File::from_raw_fd(consumer_payload_fd_read) };

    let channel = Channel::new(&executor, consumer_file, producer_file);
    let payload_channel =
        PayloadChannel::new(&executor, consumer_payload_file, producer_payload_file);
    let buffer_worker_messages_guard = channel.buffer_messages_for(std::process::id().into());

    std::thread::Builder::new()
        .name(format!("mediasoup-worker-{}", id))
        .spawn(move || {
            let argc = args.len() as c_int;
            let args_cstring = args
                .into_iter()
                .map(|s| -> CString { CString::new(s).unwrap() })
                .collect::<Vec<_>>();
            let argv = args_cstring
                .iter()
                .map(|arg| arg.as_ptr().cast::<c_char>())
                .collect::<Vec<_>>();
            let version = CString::new(env!("CARGO_PKG_VERSION")).unwrap();
            let status_code = unsafe {
                mediasoup_sys::run_worker(
                    argc,
                    argv.as_ptr(),
                    version.as_ptr(),
                    producer_fd_read,
                    consumer_fd_write,
                    producer_payload_fd_read,
                    consumer_payload_fd_write,
                )
            };

            let _ = status_sender.send(match status_code {
                0 => Ok(()),
                1 => Err(ExitError::Generic),
                42 => Err(ExitError::Settings),
                status_code => Err(ExitError::Unknown { status_code }),
            });
        })
        .expect("Failed to spawn mediasoup-worker thread");

    WorkerRunResult {
        channel,
        payload_channel,
        status_receiver,
        buffer_worker_messages_guard,
    }
}
