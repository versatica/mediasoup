# mediasoup-types v3

Type definitions and shared data structures for the [mediasoup](https://docs.rs/mediasoup) crate.

## Overview

`mediasoup-types` provides core types, enums, and data structures used throughout the mediasoup Rust crates. This crate is intended for use by both the main mediasoup implementation and any libraries, tools, or applications that interact with mediasoup in Rust.

## Usage

Add `mediasoup-types` to your `Cargo.toml`:

```toml
[dependencies]
mediasoup-types = "X.Y.Z"
```

Import and use the types in your Rust code:

```rust
use mediasoup_types::{RtpCodecParameters, RtpCapabilities, MediaKind};
```

## Documentation

- [API Documentation](https://docs.rs/mediasoup-types)
- [mediasoup Rust repository](https://github.com/versatica/mediasoup)

## Sponsor

You can support mediasoup by [sponsoring](https://mediasoup.org/sponsor) it. Thanks!

## License

[ISC](./LICENSE)
