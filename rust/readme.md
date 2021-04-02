# Rust port of [mediasoup](https://github.com/versatica/mediasoup) TypeScript library
[![Crates.io](https://img.shields.io/crates/v/mediasoup?style=flat-square)](https://crates.io/crates/mediasoup)
[![Docs](https://img.shields.io/badge/docs-latest-blue.svg?style=flat-square)](https://docs.rs/mediasoup)
[![License](https://img.shields.io/github/license/nazar-pc/mediasoup?style=flat-square)](https://github.com/versatica/mediasoup/v3/rust)

# Support forum
* [mediasoup.discourse.group](https://mediasoup.discourse.group/)

# Design Goals
mediasoup and its client side libraries are designed to accomplish with the following goals:
* Be a [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Support both WebRTC and plain RTP input and output.
* Be a Rust/Node.js module in server side.
* Be a tiny JavaScript and C++ libraries in client side.
* Be minimalist: just handle the media layer.
* Be signaling agnostic: do not mandate any signaling protocol.
* Be super low level API.
* Support all existing WebRTC endpoints.
* Enable integration with well known multimedia libraries/tools.

## Use Cases
mediasoup and its client side libraries provide a super low level API. They are intended to
enable different use cases and scenarios, without any constraint or assumption. Some of these
 use cases are:
* Group video chat applications.
* One-to-many (or few-to-many) broadcasting applications in real-time.
* RTP streaming.

## Features
* Idiomatic Rust/ECMAScript 6 low level API.
* Multi-stream: multiple audio/video streams over a single ICE + DTLS transport.
* IPv6 ready.
* ICE / DTLS / RTP / RTCP over UDP and TCP.
* Simulcast and SVC support.
* Congestion control.
* Sender and receiver bandwidth estimation with spatial/temporal layers distribution algorithm.
* Data message exchange (via WebRTC DataChannels, SCTP over plain UDP, and direct termination in Rust/Node.js).
* Extremely powerful (media worker subprocess coded in C++ on top of [libuv](https://libuv.org)).

## Authors
* Iñaki Baz Castillo [[website](https://inakibaz.me)|[github](https://github.com/ibc/)]
* José Luis Millán [[github](https://github.com/jmillan/)]
* Nazar Mokrynskyi (Rust port) [[github](https://github.com/nazar-pc)]

## License
ISC
