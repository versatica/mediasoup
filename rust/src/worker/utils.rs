mod channel_read_fn;
mod channel_write_fn;

// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use crate::worker::channel::BufferMessagesGuard;
use crate::worker::{Channel, PayloadChannel, WorkerId};
use async_executor::Executor;
use async_fs::File;
use async_oneshot::Receiver;
pub(super) use channel_read_fn::{prepare_channel_read_fn, PreparedChannelRead};
pub(super) use channel_write_fn::{
    prepare_channel_write_fn, prepare_payload_channel_write_fn, PreparedChannelWrite,
    PreparedPayloadChannelWrite,
};
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
            panic!("libc::pipe() failed: {:?}", std::io::Error::last_os_error());
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
    thread_initializer: Option<Arc<dyn Fn() + Send + Sync>>,
    executor: Arc<Executor<'static>>,
    args: Vec<String>,
) -> WorkerRunResult {
    let [producer_payload_fd_read, producer_payload_fd_write] = pipe();
    let [consumer_payload_fd_read, consumer_payload_fd_write] = pipe();
    let (mut status_sender, status_receiver) = async_oneshot::oneshot();

    let producer_payload_file = unsafe { File::from_raw_fd(producer_payload_fd_write) };
    let consumer_payload_file = unsafe { File::from_raw_fd(consumer_payload_fd_read) };

    let (channel, prepared_channel_read, prepared_channel_write) = Channel::new();
    let (payload_channel, prepared_payload_channel_write) =
        PayloadChannel::new(&executor, consumer_payload_file, producer_payload_file);
    let buffer_worker_messages_guard = channel.buffer_messages_for(std::process::id().into());

    std::thread::Builder::new()
        .name(format!("mediasoup-worker-{}", id))
        .spawn(move || {
            if let Some(thread_initializer) = thread_initializer {
                thread_initializer();
            }
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
                let (channel_read_fn, channel_read_ctx, _channel_write_callback) =
                    prepared_channel_read.deconstruct();
                let (channel_write_fn, channel_write_ctx, _channel_read_callback) =
                    prepared_channel_write.deconstruct();
                let (
                    payload_channel_write_fn,
                    payload_channel_write_ctx,
                    _payload_channel_read_callback,
                ) = prepared_payload_channel_write.deconstruct();

                mediasoup_sys::run_worker(
                    argc,
                    argv.as_ptr(),
                    version.as_ptr(),
                    0,
                    0,
                    producer_payload_fd_read,
                    consumer_payload_fd_write,
                    channel_read_fn,
                    channel_read_ctx,
                    channel_write_fn,
                    channel_write_ctx,
                    payload_channel_write_fn,
                    payload_channel_write_ctx,
                )
            };

            println!("Status sent {}", status_code);

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
