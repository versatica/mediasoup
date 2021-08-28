pub(super) use mediasoup_sys::{ChannelReadCtx, ChannelReadFn};
use mediasoup_sys::{ChannelReadFreeFn, UvAsyncT};
use std::mem;
use std::os::raw::c_void;

pub(super) struct ChannelReadCallback(
    Box<dyn (FnMut(UvAsyncT) -> Option<Vec<u8>>) + Send + 'static>,
);

pub(crate) struct PreparedChannelRead {
    channel_read_fn: ChannelReadFn,
    channel_read_ctx: ChannelReadCtx,
    write_callback: ChannelReadCallback,
}

unsafe impl Send for PreparedChannelRead {}

impl PreparedChannelRead {
    /// SAFETY:
    /// 1) `ChannelReadCallback` returned must be dropped AFTER last usage of returned function and
    ///    context pointers
    /// 2) `ChannelReadCtx` should not be called from multiple threads concurrently
    pub(super) unsafe fn deconstruct(self) -> (ChannelReadFn, ChannelReadCtx, ChannelReadCallback) {
        let Self {
            channel_read_fn,
            channel_read_ctx,
            write_callback,
        } = self;
        (channel_read_fn, channel_read_ctx, write_callback)
    }
}

unsafe extern "C" fn free_vec(message: *mut u8, message_len: u32, message_capacity: usize) {
    Vec::from_raw_parts(message, message_len as usize, message_capacity);
}

/// Given callback function, prepares a pair of channel read function and context, which can be
/// provided to of C++ worker and worker will effectively call the callback whenever it wants to
/// read something from Rust (so it is reading from C++ point of view and writing from Rust).
pub(crate) fn prepare_channel_read_fn<F>(read_callback: F) -> PreparedChannelRead
where
    F: (FnMut(UvAsyncT) -> Option<Vec<u8>>) + Send + 'static,
{
    unsafe extern "C" fn wrapper<F>(
        message: *mut *mut u8,
        message_len: *mut u32,
        message_capacity: *mut usize,
        handle: UvAsyncT,
        ChannelReadCtx(ctx): ChannelReadCtx,
    ) -> ChannelReadFreeFn
    where
        F: (FnMut(UvAsyncT) -> Option<Vec<u8>>) + Send + 'static,
    {
        // Call Rust and try to get a new message (if there is any)
        let mut new_message = (*(ctx as *mut F))(handle)?;

        // Set pointers, give out control over memory to C++
        *message = new_message.as_mut_ptr();
        *message_len = new_message.len() as u32;
        *message_capacity = new_message.capacity();

        // Forget about vector in Rust
        mem::forget(new_message);

        // Function pointer that C++ can use to free vector later
        Some(free_vec)
    }

    // Move to heap to make sure it doesn't change address later on
    let read_callback = Box::new(read_callback);

    PreparedChannelRead {
        channel_read_fn: wrapper::<F>,
        channel_read_ctx: ChannelReadCtx(read_callback.as_ref() as *const F as *const c_void),
        write_callback: ChannelReadCallback(read_callback),
    }
}
