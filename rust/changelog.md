# Changelog

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
