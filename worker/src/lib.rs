use std::ffi::CString;
use std::os::raw::{c_char, c_int};

mod ffi {
    use std::os::raw::{c_char, c_int};

    #[link(name = "mediasoup-worker", kind = "static")]
    extern "C" {
        pub fn run(
            argc: c_int,
            argv: *const *const c_char,
            version: *const c_char,
            consumer_channel_fd: c_int,
            producer_channel_fd: c_int,
            payload_consumer_channel_fd: c_int,
            payload_producer_channel_fd: c_int,
        ) -> c_int;
    }
}

// TODO: thiserror and stuff
#[derive(Debug)]
pub enum MediasoupWorkerError {
    Generic,
    Settings,
    Unknown { status_code: i32 },
}

// TODO: handle MS_ABORT nicely (currently shuts down the whole app)
/// Run mediasoup-worker in a thread
pub fn run(
    args: Vec<String>,
    consumer_channel_fd: i32,
    producer_channel_fd: i32,
    payload_consumer_channel_fd: i32,
    payload_producer_channel_fd: i32,
) -> Result<(), MediasoupWorkerError> {
    let argc = args.len() as c_int;
    let args_cstring = args
        .into_iter()
        .map(|s| -> CString { CString::new(s).unwrap() })
        .collect::<Vec<_>>();
    let argv = args_cstring
        .iter()
        .map(|arg| arg.as_ptr() as *const c_char)
        .collect::<Vec<_>>();
    let version = CString::new(env!("CARGO_PKG_VERSION")).unwrap();
    let status_code = unsafe {
        ffi::run(
            argc,
            argv.as_ptr(),
            version.as_ptr(),
            consumer_channel_fd,
            producer_channel_fd,
            payload_consumer_channel_fd,
            payload_producer_channel_fd,
        )
    };

    match status_code {
        0 => Ok(()),
        1 => Err(MediasoupWorkerError::Generic),
        42 => Err(MediasoupWorkerError::Settings),
        status_code => Err(MediasoupWorkerError::Unknown { status_code }),
    }
}
