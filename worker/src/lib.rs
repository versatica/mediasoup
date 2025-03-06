use std::os::raw::{c_char, c_int, c_void};

pub use planus_codegen::*;

mod planus_codegen {
    #![allow(clippy::all)]
    include!(concat!(env!("OUT_DIR"), "/fbs.rs"));
}

#[repr(transparent)]
#[derive(Copy, Clone)]
pub struct UvAsyncT(pub *const c_void);

unsafe impl Send for UvAsyncT {}

#[repr(transparent)]
pub struct ChannelReadCtx(pub *const c_void);
pub type ChannelReadFreeFn = Option<
    unsafe extern "C" fn(
        /* message: */ *mut u8,
        /* message_len: */ u32,
        /* message_ctx: */ usize,
    ),
>;
pub type ChannelReadFn = unsafe extern "C" fn(
    /* message: */ *mut *mut u8,
    /* message_len: */ *mut u32,
    /* message_ctx: */ *mut usize,
    // This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is
    // more data to read
    /* handle */
    UvAsyncT,
    /* ctx: */ ChannelReadCtx,
) -> ChannelReadFreeFn;

unsafe impl Send for ChannelReadCtx {}

#[repr(transparent)]
pub struct ChannelWriteCtx(pub *const c_void);
pub type ChannelWriteFn = unsafe extern "C" fn(
    /* message: */ *const u8,
    /* message_len: */ u32,
    /* ctx: */ ChannelWriteCtx,
);

unsafe impl Send for ChannelWriteCtx {}

#[cfg(feature = "worker")]
#[link(name = "mediasoup-worker", kind = "static")]
extern "C" {
    /// Returns `0` on success, or an error code `< 0` on failure
    pub fn uv_async_send(handle: UvAsyncT) -> c_int;

    pub fn mediasoup_worker_run(
        argc: c_int,
        argv: *const *const c_char,
        version: *const c_char,
        consumer_channel_fd: c_int,
        producer_channel_fd: c_int,
        channel_read_fn: ChannelReadFn,
        channel_read_ctx: ChannelReadCtx,
        channel_write_fn: ChannelWriteFn,
        channel_write_ctx: ChannelWriteCtx,
    ) -> c_int;
}

/// A stub implementation of uv_async_send when the worker feature is disabled
///
/// # Safety
///
/// This is a stub implementation that is safe to call with any value when the worker feature is disabled.
#[cfg(not(feature = "worker"))]
pub unsafe fn uv_async_send(_handle: UvAsyncT) -> c_int {
    43
}

/// A stub implementation of mediasoup_worker_run when the worker feature is disabled
///
/// # Safety
///
/// This is a stub implementation that is safe to call with any values when the worker feature is disabled.
#[cfg(not(feature = "worker"))]
#[allow(clippy::too_many_arguments)]
pub unsafe fn mediasoup_worker_run(
    _argc: c_int,
    _argv: *const *const c_char,
    _version: *const c_char,
    _consumer_channel_fd: c_int,
    _producer_channel_fd: c_int,
    _channel_read_fn: ChannelReadFn,
    _channel_read_ctx: ChannelReadCtx,
    _channel_write_fn: ChannelWriteFn,
    _channel_write_ctx: ChannelWriteCtx,
) -> c_int {
    43
}
