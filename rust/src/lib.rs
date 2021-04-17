#![warn(rust_2018_idioms, missing_debug_implementations, missing_docs)]
//! Rust port of [mediasoup](https://github.com/versatica/mediasoup) TypeScript library!
//!
//! For general information go to readme in repository.
//!
//! # For TypeScript users
//! If you were using mediasoup in TypeScript before, most of the API should be familiar to you.
//! However, this is not one-to-one port, API was adjusted to more idiomatic Rust style leveraging
//! powerful type system and ownership system to make API more robust and more misuse-resistant.
//!
//! So you will find specific types in most places where plain strings were used, instead of
//! `close()` you will see `Drop` implementation for major entities that will close everything
//! gracefully when it goes out of scope.
//!
//! # Before you start
//! This is very low-level **library**. Which means it doesn't come with a ready to use signaling
//! mechanism or easy to customize app scaffold (see
//! [design goals](https://github.com/versatica/mediasoup/tree/v3/rust/readme.md#design-goals)).
//!
//! It is recommended to visit mediasoup website and read
//! [design overview](https://mediasoup.org/documentation/v3/mediasoup/design/) first.
//!
//! There are some requirements for building underlying C++ `mediasoup-worker`, please find them in
//! [installation instructions](https://mediasoup.org/documentation/v3/mediasoup/installation/)
//!
//! # Examples
//! There are some examples in `examples` and `examples-frontend` directories (for server- and
//! client-side respectively), you may want to look at those to get a general idea of what API looks
//! like and what needs to be done in what order (check WebSocket messages in browser DevTools for
//! better understanding of what is happening under the hood).
//!
//! # How to start
//! With that in mind, you want start with creating [`WorkerManager`](worker_manager::WorkerManager)
//! instance and then 1 or more workers. Workers a responsible for low-level job of sending media
//! and data back and forth. Each worker is backed by single-core C++ worker thread. On each worker
//! you create one or more routers that enable injection, selection and forwarding of media and data
//! through [`transport`] instances. There are a few different transports available, but most likely
//! you'll want to use [`WebRtcTransport`](webrtc_transport::WebRtcTransport) most often. With
//! transport created you can start creating [`Producer`](producer::Producer)s to send data to
//! [`Router`](router::Router) and [`Consumer`](consumer::Consumer) instances to extract data from
//! [`Router`](router::Router).
//!
//! Some of the more advanced cases involve multiple routers and even workers that can user more
//! than one core on the machine or even scale beyond single host. Check
//! [scalability page](https://mediasoup.org/documentation/v3/scalability/) of the official
//! documentation.
//!
//! Please check integration and unit tests for usage examples, they cover all major functionality
//! and are a good place to start until we have demo apps built in Rust).

pub mod data_structures;
mod macros;
mod messages;
mod ortc;
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
pub use router::direct_transport;
#[cfg(doc)]
#[path = "router/direct_transport.rs"]
pub mod direct_transport;

#[cfg(not(doc))]
pub use router::pipe_transport;
#[cfg(doc)]
#[path = "router/pipe_transport.rs"]
pub mod pipe_transport;

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
