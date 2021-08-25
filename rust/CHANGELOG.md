# Changelog

# 0.8.4

* Add Active Speaker Observer to prelude
* Fix consumers preventing producers from being closed (regression introduced in 0.8.3)

# 0.8.3

* prelude module containing traits and structs that should be sufficient for most basic mediasoup-based apps
* Dominant Speaker Event (PR #603 by @SteveMcFarlin).

### 0.8.2

* Support for optional fixed port on transports

### 0.8.1

* Add convenience methods for getting information from `TransportTuple` enum, especially local IP/port
* Add `mid` option in `ConsumerOptions` to provide way to override MID
* Add convenience method `ConsumerStats::cosumer_stat()`

### 0.8.0

* `NonClosingProducer` removed (use `PipedProducer` instead, they were identical)
* `RtpHeaderExtensionUri::as_str()` now takes `self` instead of `&self`
* `kind` field of `RtpHeaderExtension` is no longer optional
* Refactor `ScalabilityMode` from being a string to enum, make sure layers are not zero on type system level
* Concrete types for info field of tracing events

### 0.7.2

* Thread and memory safety fixes in mediasoup-sys
* macOS support
* `NonClosingProducer` renamed into `PipedProducer` with better docs
* Internal restructuring of modules for better compatibility with IDEs
* Feature level updated to mediasoup `3.7.6`

### 0.7.0

* Switch from running C++ worker processes to worker threads using mediasoup-sys that wraps mediasoup-worker into library
* Simplify `WorkerManager::new()` and `WorkerManager::with_executor()` API as the result of above
* Support `rtxPacketsDiscarded` in `Producer` stats
* Enable Rust 2018 idioms warnings
* Make sure all public types have `Debug` implementation on them
* Enforce docs on public types and add missing documentation
* Remove `RtpCodecParametersParameters::new()` (`RtpCodecParametersParameters::default()` does the same thing)

### 0.6.0

Initial upstreamed release.
