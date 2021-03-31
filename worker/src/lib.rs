use std::os::raw::{c_char, c_int};

#[link(name = "mediasoup-worker", kind = "static")]
extern "C" {
    pub fn run_worker(
        argc: c_int,
        argv: *const *const c_char,
        version: *const c_char,
        consumer_channel_fd: c_int,
        producer_channel_fd: c_int,
        payload_consumer_channel_fd: c_int,
        payload_producer_channel_fd: c_int,
    ) -> c_int;
}
