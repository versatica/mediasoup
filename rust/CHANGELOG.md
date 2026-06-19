# Changelog

### NEXT

- Worker: Replace `uint64_t` hash with `TupleKey` in `TransportTuple` to avoid hash collisions ([1823](https://github.com/versatica/mediasoup/pull/1823)).
- Worker: Fix `SeqManager::GetMaxOutput()` ([1840](https://github.com/versatica/mediasoup/pull/1840)).

### 0.22.8

- Worker `Consumer`: Fix crash when using simple producer stream mode ([PR #1835](https://github.com/versatica/mediasoup/pull/1835)). The bug was introduced in version 3.20.6.

### 0.22.7

- Worker: Remove `io_uring` support ([PR #1832](https://github.com/versatica/mediasoup/pull/1832)).

### 0.22.6

- Fix `connect()` method in `PlainTransport` when comedia mode and SRTP mode are enabled ([PR #1831](https://github.com/versatica/mediasoup/pull/1831)).

### 0.22.5

- Worker: Unify all `Consumer` classes into a single one ([PR #1731](https://github.com/versatica/mediasoup/pull/1731)).
- SCTP: Validate CRC32c checksum in received packets ([PR #1828](https://github.com/versatica/mediasoup/pull/1828)).
- SCTP: Authenticate State Cookie in plain and pipe transports ([PR #1829](https://github.com/versatica/mediasoup/pull/1829)).

### 0.22.4

- Worker: `SeqManager`, use `std::vector` rather than `std::set` ([PR #1807](https://github.com/versatica/mediasoup/pull/1807)).
- SCTP: Fixes in `Association.cpp`, `StreamResetHandler.cpp` and `DataTracker.cpp` ([PR #1826](https://github.com/versatica/mediasoup/pull/1826)).

### 0.22.3

- SCTP: Fix `HeartbeatHandler` timers and close the association with `TOO_MANY_RETRIES` error when t3-rtx timer, heartbeat-timeout timer and RE-CONFIG timer expire ([PR #1824](https://github.com/versatica/mediasoup/pull/1824)).
- SCTP: Fix `ReassemblyQueue::EnterDeferredReset()` ([PR #1825](https://github.com/versatica/mediasoup/pull/1825)).

### 0.22.2

- Fix SCTP crash when the t3-rtx timer expires ([PR #1822](https://github.com/versatica/mediasoup/pull/1822)).

### 0.22.1

- `DataConsumer::send()`: Return the current buffered amount size (in bytes) after sending/queuing the message ([PR #1819](https://github.com/versatica/mediasoup/pull/1819)):
  - Fix: move the `send()` method from `DirectDataConsumer` to `RegularDataConsumer` (where the worker actually accepts it).

### 0.22.0

- New built-in SCTP stack ([PR #1806](https://github.com/versatica/mediasoup/pull/1806)):
  - Remove `useBuiltInSctpStack` worker option.
  - `WebRtcTransport`, `PlainTransport`, `PipeTransport`: Add `sctp_negotiated_capabilities()` getter.
  - `WebRtcTransport`, `PlainTransport`, `PipeTransport` options: Remove `num_sctp_streams` and `max_sctp_message_size`, and add `max_send_message_size`, `max_receive_message_size`, `sctp_per_stream_send_queue_limit` and `sctp_max_receiver_window_buffer_size`.
  - `DirectTransport` options: Remove `max_message_size`, and add `max_send_message_size` and `max_receive_message_size`.
  - Change `SctpParameters` type from `{ port, OS, MIS, maxMessageSize }` to `{ port, max_send_message_size, max_receive_message_size, send_buffer_size, per_stream_send_queue_limit,max_receiver_window_buffer_size, is_data_channel}`.

### 0.21.0

- `router.pipe_producer_to_router()` and `router.pipe_data_producer_to_router()` can now connect two `Routers` in the same `Worker` if `keep_id` is set to `false` ([PR #1604](https://github.com/versatica/mediasoup/pull/1604)).
- Updates from mediasoup TypeScript `3.18.1.=3.19.0`.
- Ensure that the order of acquiring the `paused` and `producer_paused` locks in `consumer.rs` is consistent at all times to avoid deadlock ([PR #1605](https://github.com/versatica/mediasoup/pull/1605)).
- Convert `WORKER_CLOSE` into a notification ([PR #1729](https://github.com/versatica/mediasoup/pull/1729)).

### 0.20.0

- Make `parameters` and `rtcp_feedback` optional in `RtpCodecParameters` and `RtpCodecCapability` during deserialization ([PR #1597](https://github.com/versatica/mediasoup/pull/1597)).
- Make codec `mime_type` case insensitive during deserialization ([PR #1599](https://github.com/versatica/mediasoup/pull/1599)).
- Only expose `data_structures`, `rtp_parameters`, `sctp_parameters` and `srtp_parameters` through the `mediasoup-types` crate ([PR #1600](https://github.com/versatica/mediasoup/pull/1600)).

### 0.19.1

- Fix installation in paths with spaces ([PR #1596](https://github.com/versatica/mediasoup/pull/1596)).
- Updates from mediasoup TypeScript `3.17.1.=3.18.1`.

### 0.19.0

- Enable AV1 codec ([PR #1563](https://github.com/versatica/mediasoup/pull/1563)).
- Remove H265 codec and deprecated frame-marking RTP extension ([PR #1564](https://github.com/versatica/mediasoup/pull/1564)).
- Remove H264-SVC codec ([PR #1568](https://github.com/versatica/mediasoup/pull/1568)).
- Add `Router::update_media_codecs()` to dynamically change Router's RTP capabilities (#1571).
- `TransportListenInfo`: Add `expose_internal_ip` which, if set to `true` and `announced_address` is set, exposes an additional ICE candidate in `WebRtcTransport` whose IP is `listen_info.ip` rather than `listen_info.announced_address` ([PR #1583](https://github.com/versatica/mediasoup/pull/1583)).
- Updates from mediasoup TypeScript `3.14.11.=3.17.0`.

### 0.18.2

- Don't log error if `close()` on an object fails because channel is closed already ([PR #1560](https://github.com/versatica/mediasoup/pull/1560)).
- General mediasoup changes:
  - Sign self generated DTLS certificate with SHA256 ([PR #1450](https://github.com/versatica/mediasoup/pull/1450)).
  - `SimulcastConsumer`: Fix cannot switch layers if initial `tsReferenceSpatialLayer disappears` disappears ([PR #1459](https://github.com/versatica/mediasoup/pull/1459)).
  - Worker: Fix crash when using colliding `portRange` values in different transports ([PR #1469](https://github.com/versatica/mediasoup/pull/1469)).
  - Worker: Drop VP8 packets with a higher temporal layer than the current one ([PR #1009](https://github.com/versatica/mediasoup/pull/1009)).
  - Fix the problem of the TCC package being omitted from being sent ([PR #1492](https://github.com/versatica/mediasoup/pull/1492)).
  - `Consumer`: Fix sequence number gap ([PR #1494](https://github.com/versatica/mediasoup/pull/1494)).
  - Fix VP9 out of order packets forwarding ([PR #1486](https://github.com/versatica/mediasoup/pull/1486)).
  - Fix wrong SCTP stream parameters in SCTP `DataConsumer` that consumes from a direct `DataProducer` ([PR #1516](https://github.com/versatica/mediasoup/pull/1516)).
  - Worker: Fix encode retransmitted packets with the corresponding data ([PR #1527](https://github.com/versatica/mediasoup/pull/1527)).
  - `SvcConsumer`: Fix K-SVC bitrate in `IncreaseLayer()` method ([PR #1535](https://github.com/versatica/mediasoup/pull/1535)).
  - `Consumer` classes: Only drop packets in RTP sequence manager when they belong to current spatial layer ([PR #1549](https://github.com/versatica/mediasoup/pull/1549)).
  - `Consumer` classes: Add target layer retransmission buffer to avoid PLIs/FIRs when RTP packets containing a key frame arrive out of order ([PR #1550](https://github.com/versatica/mediasoup/pull/1550) and [PR #1558](https://github.com/versatica/mediasoup/pull/1558)).

### 0.18.1

- FBS: Provide proper data upon panic (#1523).

### 0.18.0

- Fix wrong SCTP stream parameters in SCTP `DataConsumer` that consumes from a direct `DataProducer` ([PR #1516](https://github.com/versatica/mediasoup/pull/1516)).
- New enum variant was added in 0.17.2.

### 0.17.2

- Fix `PipeConsumer::get_stats()` ([PR #1511](https://github.com/versatica/mediasoup/pull/1511)).

### 0.17.1

- Update Rust toolchain channel to version 1.79.0 ([PR #1409](https://github.com/versatica/mediasoup/pull/1409)).
- Updates from mediasoup TypeScript `3.14.7..=3.14.10`.
- General mediasoup changes:
  - Worker: Add `enable_liburing` boolean option (`true` by default) to disable `io_uring` even if it's supported by the prebuilt `mediasoup-worker` and by current host ([PR #1442](https://github.com/versatica/mediasoup/pull/1442)).

### 0.17.0

- Updates from mediasoup TypeScript `3.13.18..=3.14.6`.
- General mediasoup changes:
  - Worker: Fix crash when closing `WebRtcServer` with active `WebRtcTransports` ([PR #1390](https://github.com/versatica/mediasoup/pull/1390)).
  - `Worker: Fix memory leak when using `WebRtcServer` with TCP enabled ([PR #1389](https://github.com/versatica/mediasoup/pull/1389)).
  - OPUS: Fix DTX detection ([PR #1357](https://github.com/versatica/mediasoup/pull/1357)).
  - `TransportListenInfo`: Add `portRange` (deprecate worker port range) ([PR #1365](https://github.com/versatica/mediasoup/pull/1365)).
  - Update worker FlatBuffers to 24.3.6-1 (fix cannot set temporal layer 0) ([PR #1348](https://github.com/versatica/mediasoup/pull/1348)).
  - Fix DTLS packets do not honor configured DTLS MTU (attempt 3) ([PR #1345](https://github.com/versatica/mediasoup/pull/1345)).
  - Add server side ICE consent checks to detect silent WebRTC disconnections ([PR #1332](https://github.com/versatica/mediasoup/pull/1332)).
  - `TransportListenInfo`: "announced ip" can also be a hostname ([PR #1322](https://github.com/versatica/mediasoup/pull/1322)).
  - `TransportListenInfo`: Rename "announced ip" to "announced address" ([PR #1324](https://github.com/versatica/mediasoup/pull/1324)).

### 0.16.0

- Updates from mediasoup TypeScript `3.13.13..=3.13.17`.
- General mediasoup changes:
  - `TransportListenInfo.announced_ip` can also be a hostname ([PR #1322](https://github.com/versatica/mediasoup/pull/1322)).
  - `TransportListenInfo.announced_ip` is now `announced_address`, `IceCandidate.ip` is now `IceCandidate.address` and `TransportTuple.local_ip` is not `TransportTuple.local_address` ([PR #1324](https://github.com/versatica/mediasoup/pull/1324)).

### 0.15.0

- Expose DataChannel string message as binary ([PR #1289](https://github.com/versatica/mediasoup/pull/1289)).

### 0.14.0

- Updates from mediasoup TypeScript `3.13.8..=3.13.12`.
- Update h264-profile-level-id dependency to 0.2.0.
- Fix docs build ([PR #1271](https://github.com/versatica/mediasoup/pull/1271)).
- Rename `data_consumer::on_producer_resume` to `data_consumer::on_data_producer_resume` ([PR #1271](https://github.com/versatica/mediasoup/pull/1271)).

### 0.13.0

- Updates from mediasoup TypeScript `3.13.0..=3.13.7`.
- General mediasoup changes:
  - Switch from JSON based messages to `flatbuffers` ([PR #1064](https://github.com/versatica/mediasoup/pull/1064)).
  - Enable `liburing` usage for Linux (kernel versions >= 6) ([PR #1218](https://github.com/versatica/mediasoup/pull/1218)).
  - Add pause/resume API in `DataProducer` and `DataConsumer` ([PR #1104](https://github.com/versatica/mediasoup/pull/1104)).
  - DataChannel subchannels feature ([PR #1152](https://github.com/versatica/mediasoup/pull/1152)).
  - `Worker`: Make DTLS fragment stay within MTU size range ([PR #1156](https://github.com/versatica/mediasoup/pull/1156)).
  - Replace make + Makefile with Python Invoke library + tasks.py (also fix installation under path with whitespaces) ([PR #1239](https://github.com/versatica/mediasoup/pull/1239)).

### 0.12.0

- Updates from mediasoup TypeScript `3.11.9..=3.12.16`.

### 0.11.4

- Fix consuming data producer from direct transport by data consumer on non-direct transport.

### 0.11.3

- Updates from mediasoup TypeScript `3.11.3..=3.11.8`.

### 0.11.2

- Updates from mediasoup TypeScript `3.10.11..=3.11.2`.

### 0.11.1

- Updates from mediasoup TypeScript `3.10.7..=3.10.10`.

### 0.11.0

- Updates from mediasoup TypeScript `3.10.2..=3.10.6`.

### 0.10.0

- Updates from mediasoup TypeScript `3.9.10..=3.10.1`.
- `WebRtcServer`: A new class that brings to `WebRtcTransports` the ability to listen on a single UDP/TCP port ([PR #834](https://github.com/versatica/mediasoup/pull/834), [PR #845](https://github.com/versatica/mediasoup/pull/845)).
- Minor API breaking changes.

### 0.9.3

- Fix a segfaults in tests and under multithreaded executor.
- Fix another racy deadlock situation.
- Expose hierarchical dependencies of ownership of Rust data structures, now it is possible to call `consumer.transport().router().worker().worker_manager()`.
- General mediasoup changes:
  - ICE renomination support ([PR #756](https://github.com/versatica/mediasoup/pull/756)).
  - Update `libuv` to 1.43.0.
  - TCC client optimizations for faster and more stable BWE ([PR #712](https://github.com/versatica/mediasoup/pull/712) by @ggarber).
  - Added support for RTP abs-capture-time header ([PR #761](https://github.com/versatica/mediasoup/pull/761) by @oto313).
  - Fix VP9 kSVC forwarding logic to not forward lower unneded layers ([PR #778](https://github.com/versatica/mediasoup/pull/778) by @ggarber).
  - Fix update bandwidth estimation configuration and available bitrate when updating max outgoing bitrate ([PR #779](https://github.com/versatica/mediasoup/pull/779) by @ggarber).
  - Optimize RTP header extension handling ([PR #786](https://github.com/versatica/mediasoup/pull/786)).
  - `RateCalculator`: Reset optimization ([PR #785](https://github.com/versatica/mediasoup/pull/785)).
  - Fix frozen video due to double call to `Consumer::UserOnTransportDisconnected()` ([PR #788](https://github.com/versatica/mediasoup/pull/788), thanks to @ggarber for exposing this issue in [PR #787](https://github.com/versatica/mediasoup/pull/787)).

### 0.9.2

- Update `lru` dependency to fix security vulnerability

### 0.9.1

- Fix cleanup of build artifacts.
- Make `Transport` implement `Send`.
- Another fix to rare deadlock.
- Improved Windows support (doesn't require MSVS activation).

### 0.9.0

- Fix for receiving data over payload channel.
- Support thread initializer function for worker threads, can be used for pinning worker threads to CPU cores.
- Significant worker communication optimizations (especially latency).
- Switch from file descriptors to function calls when communicating with worker.
- Various optimizations that caused minor breaking changes to public API.
- Requests no longer have internal timeout, but they can now be cancelled, add your own timeouts on top if needed.
- Windows support.
- General mediasoup changes:
  - Replaces GYP build system with fully-functional Meson build system ([PR #622](https://github.com/versatica/mediasoup/pull/622)).
  - `Consumer`: Modification of bitrate allocation algorithm ([PR #708](https://github.com/versatica/mediasoup/pull/708)).
  - Single H264/H265 codec configuration in `supportedRtpCapabilities` ([PR #718](https://github.com/versatica/mediasoup/pull/718)).

### 0.8.5

- Fix types for `round_trip_time` and `bitrate_by_layer` fields `ProducerStat` and `ConsumerStat`.
- Accumulation of worker fixes.

### 0.8.4

- Add Active Speaker Observer to prelude.
- Fix consumers preventing producers from being closed (regression introduced in 0.8.3).

### 0.8.3

- prelude module containing traits and structs that should be sufficient for most basic mediasoup-based apps.
- Dominant Speaker Event ([PR #603](https://github.com/versatica/mediasoup/pull/603) by @SteveMcFarlin).

### 0.8.2

- Support for optional fixed port on transports.

### 0.8.1

- Add convenience methods for getting information from `TransportTuple` enum, especially local IP/port.
- Add `mid` option in `ConsumerOptions` to provide way to override MID
- Add convenience method `ConsumerStats::consumer_stat()`.

### 0.8.0

- `NonClosingProducer` removed (use `PipedProducer` instead, they were identical).
- `RtpHeaderExtensionUri::as_str()` now takes `self` instead of `&self`.
- `kind` field of `RtpHeaderExtension` is no longer optional.
- Refactor `ScalabilityMode` from being a string to enum, make sure layers are not zero on type system level.
- Concrete types for info field of tracing events.

### 0.7.2

- Thread and memory safety fixes in mediasoup-sys.
- macOS support.
- `NonClosingProducer` renamed into `PipedProducer` with better docs.
- Internal restructuring of modules for better compatibility with IDEs.
- Feature level updated to mediasoup `3.7.6`.

### 0.7.0

- Switch from running C++ worker processes to worker threads using mediasoup-sys that wraps mediasoup-worker into library.
- Simplify `WorkerManager::new()` and `WorkerManager::with_executor()` API as the result of above.
- Support `rtxPacketsDiscarded` in `Producer` stats.
- Enable Rust 2018 idioms warnings.
- Make sure all public types have `Debug` implementation on them.
- Enforce docs on public types and add missing documentation.
- Remove `RtpCodecParametersParameters::new()` (`RtpCodecParametersParameters::default()` does the same thing).

### 0.6.0

Initial upstreamed release.
