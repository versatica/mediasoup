use std::os::raw::{c_char, c_int};

// TODO: handle MS_ABORT nicely (currently shuts down the whole app)
#[link(name = "mediasoup-worker", kind = "static")]
extern "C" {
    pub fn run_worker(
        argc: c_int,
        argv: *const *const c_char,
        version: *const c_char,
        process_mode: bool,
        consumer_channel_fd: c_int,
        producer_channel_fd: c_int,
        payload_consumer_channel_fd: c_int,
        payload_producer_channel_fd: c_int,
    ) -> c_int;
}
