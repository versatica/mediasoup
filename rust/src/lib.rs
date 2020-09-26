pub mod data_structures;
mod macros;
mod messages;
pub mod ortc;
pub mod router;
pub mod rtp_parameters;
pub mod scalability_modes;
pub mod supported_rtp_capabilities;
pub mod worker;
pub mod worker_manager;

// TODO: The mess below is because of https://github.com/rust-lang/rust/issues/59368
#[cfg(not(doc))]
pub use router::consumer;
#[cfg(doc)]
#[path = "router/consumer.rs"]
pub mod consumer;

#[cfg(not(doc))]
pub use router::producer;
#[cfg(doc)]
#[path = "router/producer.rs"]
pub mod producer;

#[cfg(not(doc))]
pub use router::transport;
#[cfg(doc)]
#[path = "router/transport.rs"]
pub mod transport;

#[cfg(not(doc))]
pub use router::webrtc_transport;
#[cfg(doc)]
#[path = "router/webrtc_transport.rs"]
pub mod webrtc_transport;
