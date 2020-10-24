//! This is a port of TypeScript client library for
//! [Mediasoup](https://github.com/versatica/mediasoup) to Rust
//!
//! The library is incomplete and at early stages of development, so it will be hard to use unless
//! you already have experience with Mediasoup (in this case you can create a worker using
//! [worker_manager::WorkerManager] and go from there).
//!
//! Also there are basic tests in `worker_manager` and `worker` modules that may help to get started.
//!
//! API is close to [TypeScript's](https://mediasoup.org/documentation/v3/mediasoup/api/), please
//! referer to it in the meantime.

pub mod data_structures;
mod macros;
mod messages;
pub mod ortc;
pub mod router;
pub mod rtp_parameters;
pub mod scalability_modes;
pub mod sctp_parameters;
pub mod srtp_parameters;
pub mod supported_rtp_capabilities;
pub mod worker;
pub mod worker_manager;

// TODO: The mess below is because of https://github.com/rust-lang/rust/issues/59368
#[cfg(not(doc))]
pub use router::audio_level_observer;
#[cfg(doc)]
#[path = "router/audio_level_observer.rs"]
pub mod audio_level_observer;

#[cfg(not(doc))]
pub use router::consumer;
#[cfg(doc)]
#[path = "router/consumer.rs"]
pub mod consumer;

#[cfg(not(doc))]
pub use router::data_consumer;
#[cfg(doc)]
#[path = "router/data_consumer.rs"]
pub mod data_consumer;

#[cfg(not(doc))]
pub use router::observer;
#[cfg(doc)]
#[path = "router/observer.rs"]
pub mod observer;

#[cfg(not(doc))]
pub use router::producer;
#[cfg(doc)]
#[path = "router/producer.rs"]
pub mod producer;

#[cfg(not(doc))]
pub use router::data_producer;
#[cfg(doc)]
#[path = "router/data_producer.rs"]
pub mod data_producer;

#[cfg(not(doc))]
pub use router::transport;
#[cfg(doc)]
#[path = "router/transport.rs"]
pub mod transport;

#[cfg(not(doc))]
pub use router::plain_transport;
#[cfg(doc)]
#[path = "router/plain_transport.rs"]
pub mod plain_transport;

#[cfg(not(doc))]
pub use router::rtp_observer;
#[cfg(doc)]
#[path = "router/rtp_observer.rs"]
pub mod rtp_observer;

#[cfg(not(doc))]
pub use router::webrtc_transport;
#[cfg(doc)]
#[path = "router/webrtc_transport.rs"]
pub mod webrtc_transport;
