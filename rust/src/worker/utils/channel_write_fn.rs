pub(super) use mediasoup_sys::{
    ChannelWriteCtx, ChannelWriteFn, PayloadChannelWriteCtx, PayloadChannelWriteFn,
};
use std::os::raw::c_void;
use std::slice;

pub(super) struct ChannelReadCallback(Box<dyn FnMut(&[u8]) + Send + 'static>);

pub(crate) struct PreparedChannelWrite {
    channel_write_fn: ChannelWriteFn,
    channel_write_ctx: ChannelWriteCtx,
    read_callback: ChannelReadCallback,
}

unsafe impl Send for PreparedChannelWrite {}

impl PreparedChannelWrite {
    /// SAFETY:
    /// 1) `ChannelReadCallback` returned must be dropped AFTER last usage of returned function and
    ///    context pointers
    /// 2) `ChannelWriteCtx` should not be called from multiple threads concurrently
    pub(super) unsafe fn deconstruct(
        self,
    ) -> (ChannelWriteFn, ChannelWriteCtx, ChannelReadCallback) {
        let Self {
            channel_write_fn,
            channel_write_ctx,
            read_callback,
        } = self;
        (channel_write_fn, channel_write_ctx, read_callback)
    }
}

/// Given callback function, prepares a pair of channel write function and context, which can be
/// provided to of C++ worker and worker will effectively call the callback whenever it needs to
/// send something to Rust (so it is writing from C++ point of view and reading from Rust).
pub(crate) fn prepare_channel_write_fn<F>(read_callback: F) -> PreparedChannelWrite
where
    F: FnMut(&[u8]) + Send + 'static,
{
    unsafe extern "C" fn wrapper<F>(
        message: *const u8,
        message_len: u32,
        ChannelWriteCtx(ctx): ChannelWriteCtx,
    ) where
        F: FnMut(&[u8]) + Send + 'static,
    {
        let message = slice::from_raw_parts(message, message_len as usize);
        (*(ctx as *mut F))(message);
    }

    // Move to heap to make sure it doesn't change address later on
    let read_callback = Box::new(read_callback);

    PreparedChannelWrite {
        channel_write_fn: wrapper::<F>,
        channel_write_ctx: ChannelWriteCtx(read_callback.as_ref() as *const F as *const c_void),
        read_callback: ChannelReadCallback(read_callback),
    }
}

pub(super) struct PayloadChannelReadCallback(Box<dyn FnMut(&[u8], &[u8]) + Send + 'static>);

pub(crate) struct PreparedPayloadChannelWrite {
    channel_write_fn: PayloadChannelWriteFn,
    channel_write_ctx: PayloadChannelWriteCtx,
    read_callback: PayloadChannelReadCallback,
}

unsafe impl Send for PreparedPayloadChannelWrite {}

impl PreparedPayloadChannelWrite {
    /// SAFETY:
    /// 1) `PayloadChannelReadCallback` returned must be dropped AFTER last usage of returned
    ///    function and context pointers
    /// 2) `PayloadChannelWriteCtx` should not be called from multiple threads concurrently
    pub(super) unsafe fn deconstruct(
        self,
    ) -> (
        PayloadChannelWriteFn,
        PayloadChannelWriteCtx,
        PayloadChannelReadCallback,
    ) {
        let Self {
            channel_write_fn,
            channel_write_ctx,
            read_callback,
        } = self;
        (channel_write_fn, channel_write_ctx, read_callback)
    }
}

/// Given callback function, prepares a pair of channel write function and context, which can be
/// provided to of C++ worker and worker will effectively call the callback whenever it needs to
/// send something to Rust (so it is writing from C++ point of view and reading from Rust).
pub(crate) fn prepare_payload_channel_write_fn<F>(read_callback: F) -> PreparedPayloadChannelWrite
where
    F: FnMut(&[u8], &[u8]) + Send + 'static,
{
    unsafe extern "C" fn wrapper<F>(
        message: *const u8,
        message_len: u32,
        payload: *const u8,
        payload_len: u32,
        PayloadChannelWriteCtx(ctx): PayloadChannelWriteCtx,
    ) where
        F: FnMut(&[u8], &[u8]) + Send + 'static,
    {
        let message = slice::from_raw_parts(message, message_len as usize);
        let payload = if payload_len == 0 {
            &[]
        } else {
            slice::from_raw_parts(payload, payload_len as usize)
        };
        (*(ctx as *mut F))(message, payload);
    }

    // Move to heap to make sure it doesn't change address later on
    let read_callback = Box::new(read_callback);

    PreparedPayloadChannelWrite {
        channel_write_fn: wrapper::<F>,
        channel_write_ctx: PayloadChannelWriteCtx(
            read_callback.as_ref() as *const F as *const c_void
        ),
        read_callback: PayloadChannelReadCallback(read_callback),
    }
}
