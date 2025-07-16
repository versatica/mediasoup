# mediasoup-types v3

Type definitions and shared data structures for the [mediasoup](https://docs.rs/mediasoup) crate.

## Overview

`mediasoup-types` provides core types, enums, and data structures used throughout the mediasoup Rust crates. This crate is intended for use by both the main mediasoup implementation and any libraries, tools, or applications that interact with mediasoup in Rust.

## Usage

Add `mediasoup-types` to your `Cargo.toml`:

```toml
[dependencies]
mediasoup-types = "0.1"
```

Import and use the types in your Rust code:

```rust
use mediasoup_types::{RtpCodecParameters, RtpCapabilities, MediaKind};
```

## Documentation

- [API Documentation](https://docs.rs/mediasoup-types)
- [mediasoup Rust repository](https://github.com/versatica/mediasoup)

## Sponsor

You can support mediasoup by [sponsoring][sponsor] it. Thanks!

## License

[ISC](./LICENSE)

[mediasoup-banner]: /art/mediasoup-banner.png
[mediasoup-website]: https://mediasoup.org
[mediasoup-discourse]: https://mediasoup.discourse.group
[npm-shield-mediasoup]: https://img.shields.io/npm/v/mediasoup.svg
[npm-mediasoup]: https://npmjs.org/package/mediasoup
[crates-shield-mediasoup]: https://img.shields.io/crates/v/mediasoup.svg
[crates-mediasoup]: https://crates.io/crates/mediasoup
[opencollective-shield-mediasoup]: https://img.shields.io/opencollective/all/mediasoup.svg
[opencollective-mediasoup]: https://opencollective.com/mediasoup
[github-actions-shield-mediasoup-node]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-node.yaml/badge.svg
[github-actions-mediasoup-node]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-node.yaml
[github-actions-shield-mediasoup-worker]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker.yaml/badge.svg
[github-actions-mediasoup-worker]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker.yaml
[github-actions-shield-mediasoup-rust]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-rust.yaml/badge.svg
[github-actions-mediasoup-rust]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-rust.yaml
[github-actions-shield-mediasoup-worker-fuzzer]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker-fuzzer.yaml/badge.svg
[github-actions-mediasoup-worker-fuzzer]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker-fuzzer.yaml
[github-actions-shield-mediasoup-worker-prebuild]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker-prebuild.yaml/badge.svg
[github-actions-mediasoup-worker-prebuild]: https://github.com/versatica/mediasoup/actions/workflows/mediasoup-worker-prebuild.yaml
[codeql-shield-mediasoup]: https://github.com/versatica/mediasoup/actions/workflows/codeql.yml/badge.svg
[codeql-mediasoup]: https://github.com/versatica/mediasoup/actions/workflows/codeql.yml
[sponsor]: https://mediasoup.org/sponsor
[mediasoup-architecture]: /art/mediasoup-v3-architecture-01.svg
[mediasoup-demo-screenshot]: /art/mediasoup-v3.png
[mediasoup-demo]: https://v3demo.mediasoup.org
