# Changelog


### 3.6.34

* Fix crash (regression, issue #529).


### 3.6.33

* Add missing `delete cb` that otherwise would leak (PR #527 based on PR #526 by @vpalmisano).
* `router.pipeToRouter()`: Fix possible inconsistency in `pipeProducer.paused` status (as discussed in this [thread](https://mediasoup.discourse.group/t/concurrency-architecture/2515/) in the mediasoup forum).
* Update `nlohmann/json` to 3.9.1.
* Update `usrsctp`.
* Update NPM deps.
* Enhance Jitter calculation.


### 3.6.32

* Fix notifications from `mediasoup-worker` being processed before responses received before them (issue #501).


### 3.6.31

* Move `bufferedAmount` from `dataConsumer.dump()` to `dataConsumer.getStats()`.
* Update NPM deps.


### 3.6.30

* Add `pipe` option to `transport.consume()`(PR #494).
  - So the receiver will get all streams from the `Producer`.
  - It works for any kind of transport (but `PipeTransport` which is always like this).
* Update NPM deps.
* Add `LICENSE` and `PATENTS` files in `libwebrtc` dependency (issue #495).
* Added `worker/src/Utils/README_BASE64_UTILS` (issue #497).
* Update `Catch` to 2.13.4.
* Update `usrsctp`.


### 3.6.29

* Fix wrong message about `rtcMinPort` and `rtcMaxPort`.
* Update deps.
* Improve `EnhancedEventEmitter.safeAsPromise()` (although not used).


### 3.6.28

* Fix replacement of `__MEDIASOUP_VERSION__` in `lib/index.d.ts` (issue #483).
* Update NPM deps.
* `worker/scripts/configure.py`: Handle 'mips64' (PR #485).


### 3.6.27

* Update NPM deps.
* Allow the `mediasoup-worker` process to inherit all environment variables (issue #480).


### 3.6.26

* BWE tweaks and debug logs.
* Update NPM deps.


### 3.6.25

* Update `Catch` to 2.13.2.
* Update NPM deps.
* sctp fixes #479.


### 3.6.24

* Update `awaitqueue` dependency.


### 3.6.23

* Fix yet another memory leak in Node.js layer due to `PayloadChannel` event listener not being removed.
* Update NPM deps.


### 3.6.22

* `Transport.cpp`: Provide transport congestion client with RTCP Receiver Reports (#464).
* Update `libuv` to 1.40.0.
* Update Node deps.
* `SctpAssociation.cpp`: increase `sctpBufferedAmount` before sending any data (#472).


### 3.6.21

* Fix memory leak in Node.js layer due to `PayloadChannel` event listener not being removed (related to #463).


### 3.6.20

* Remove `-fwrapv` when building mediasoup-worker in `Debug` mode (issue #460).
* Add `MEDIASOUP_MAX_CORES` to limit `NUM_CORES` during mediasoup-worker build (PR #462).


### 3.6.19

* Update `usrsctp` dependency.
* Update `typescript-eslint` deps.
* Update Node deps.


### 3.6.18

* Fix `ortc.getConsumerRtpParameters()` RTX codec comparison issue (PR #453).
* RtpObserver: expose `RtpObserverAddRemoveProducerOptions` for `addProducer()` and `removeProducer()` methods.


### 3.6.17

* Update `libuv` to 1.39.0.
* Update Node deps.
* SimulcastConsumer: Prefer the highest spatial layer initially (PR #450).
* RtpStreamRecv: Set RtpDataCounter window size to 6 secs if DTX (#451)


### 3.6.16

* `SctpAssociation.cpp`: Fix `OnSctpAssociationBufferedAmount()` call.
* Update deps.
* New API to send data from Node throught SCTP DataConsumer.


### 3.6.15

* Avoid SRTP leak by deleting invalid SSRCs after STRP decryption (issue #437, thanks to @penguinol for reporting).
* Update `usrsctp` dep.
* DataConsumer 'bufferedAmount' implementation (PR #442).

### 3.6.14

* Fix `usrsctp` vulnerability (PR #439).
* Fix issue #435 (thanks to @penguinol for reporting).
* `TransportCongestionControlClient.cpp`: Enable periodic ALR probing to recover faster from network issues.
* Update NPM deps.
* Update `nlohmann::json` C++ dep to 3.9.0.
* Update `Catch` to 2.13.0.


### 3.6.13

* RTP on `DirectTransport` (issue #433, PR #434):
  - New API `producer.send(rtpPacket: Buffer)`.
  - New API `consumer.on('rtp', (rtpPacket: Buffer)`.
  - New API `directTransport.sendRtcp(rtcpPacket: Buffer)`.
  - New API `directTransport.on('rtcp', (rtpPacket: Buffer)`.


### 3.6.12

* Release script.


### 3.6.11

* `Transport`: rename `maxSctpSendBufferSize` to `sctpSendBufferSize`.


### 3.6.10

* `Transport`: Implement `maxSctpSendBufferSize`.
* Update `libuv` to 1.38.1.
* Update `Catch` to 2.12.4.
* Update NPM deps.


### 3.6.9

* `Transport::ReceiveRtpPacket()`: Call `RecvStreamClosed(packet->GetSsrc())` if received RTP packet does not match any `Producer`.
* `Transport::HandleRtcpPacket()`: Ensure `Consumer` is found for received NACK Feedback packets.
* Update NPM deps.
* Update C++ `Catch` dep.
* Fix issue #408.


### 3.6.8

* Fix SRTP leak due to streams not being removed when a `Producer` or `Consumer` is closed.
  - PR #428 (fixes issues #426). 
  - Credits to credits to @penguinol for reporting and initial work at PR #427.
* Update `nlohmann::json` C++ dep to 3.8.0.
* C++: Enhance `const` correctness.
* Update NPM deps.


### 3.6.7

* `ConsumerScore`: Add `producerScores`, scores of all RTP streams in the producer ordered by encoding (just useful when the producer uses simulcast).
  - PR #421 (fixes issues #420).
* Hide worker executable console in Windows.
  - PR #419 (credits to @BlueMagnificent).
* `RtpStream.cpp`: Fix wrong `std::round()` usage.
  - Issue #423.


### 3.6.6

* Update `usrsctp` library.
* Update ESlint and TypeScript related dependencies.


### 3.6.5

* Set `score:0` when `dtx:true` is set in an `encoding` and there is no RTP for some seconds for that RTP stream.
  - Fixes #415.


### 3.6.4

* `gyp`: Fix CLT version detection in OSX Catalina when XCode app is not installed.
  - PR #413 (credits to @enimo).


### 3.6.3

* Modernize TypeScript.


### 3.6.2

* Fix crash in `Transport.ts` when closing a `DataConsumer` created on a `DirectTransport`.


### 3.6.1

* Export new `DirectTransport` in `types`.
* Make `DataProducerOptions` optional (not needed when in a `DirectTransport`).


### 3.6.0

* SCTP/DataChannel termination:
  - PR #409
  - Allow the Node application to directly send text/binary messages to mediasoup-worker C++ process so others can consume them using `DataConsumers`.
  - And vice-versa: allow the Node application to directly consume in Node messages send by `DataProducers`.
* Add `WorkerLogTag` TypeScript enum and also add a new 'message' tag into it.


### 3.5.15

* Simulcast and SVC: Better computation of desired bitrate based on `maxBitrate` field in the `producer.rtpParameters.encodings`.


### 3.5.14

* Update deps, specially `uuid` and `@types/uuid` that had a TypeScript related bug.
* `TransportCongestionClient.cpp`: Improve sender side bandwidth estimation by do not reporting `this->initialAvailableBitrate` as available bitrate due to strange behavior in the algorithm.


### 3.5.13

* Simplify `GetDesiredBitrate()` in `SimulcastConsumer` and `SvcConsumer`.
* Update libuv to 1.38.0.


### 3.5.12

* `SeqManager.cpp`: Improve performance.
  - PR #398 (credits to @penguinol).


### 3.5.11

* `SeqManager.cpp`: Fix a bug and improve performance.
  - Fixes issue #395 via PR #396 (credits to @penguinol).
* Drop Node.js 8 support. Minimum supported Node.js version is now 10.
* Upgrade `eslint` and `jest` major versions.


### 3.5.10

* `SimulcastConsumer.cpp`: Fix `IncreaseLayer()` method (fixes #394).
* Udpate Node deps.


### 3.5.9

* `libwebrtc`: Apply patch by @sspanak and @Ivaka to avoid crash. Related issue: #357.
* `PortManager.cpp`: Do not use `UV_UDP_RECVMMSG` in Windows due to a bug in libuv 1.37.0.
* Update Node deps.


### 3.5.8

* Enable `UV_UDP_RECVMMSG`:
  - Upgrade libuv to 1.37.0.
  - Use `uv_udp_init_ex()` with `UV_UDP_RECVMMSG` flag.
  - Add our own `uv.gyp` now that libuv has removed support for GYP (fixes #384).


### 3.5.7

* Fix crash in mediasoup-worker due to conversion from `uint64_t` to `int64_t` (used within `libwebrtc` code. Fixes #357.
* Update `usrsctp` library.
* Update Node deps.


### 3.5.6

* `SeqManager.cpp`: Fix video lag after a long time.
  - Fixes #372 (thanks @penguinol for reporting it and giving the solution).


### 3.5.5

* `UdpSocket.cpp`: Revert `uv__udp_recvmmsg()` usage since it notifies about received UDP packets in reverse order. Feature on hold until fixed. 


### 3.5.4

* `Transport.cpp`: Enable transport congestion client for the first video Consumer, no matter it's uses simulcast, SVC or a single stream.
* Update libuv to 1.35.0.
* `UdpSocket.cpp`: Ensure the new libuv's `uv__udp_recvmmsg()` is used, which is more efficient.


### 3.5.3

* `PlainTransport`: Remove `multiSource` option. It was a hack nobody should use.


### 3.5.2

* Enable MID RTP extension in mediasoup to receivers direction (for consumers).
  - This **requires** mediasoup-client 3.5.2 to work.


### 3.5.1

* `PlainTransport`: Fix event name: 'rtcpTuple' => 'rtcptuple'.


### 3.5.0

* `PipeTransport`: Add support for SRTP and RTP retransmission (RTX + NACK). Useful when connecting two mediasoup servers running in different hosts via pipe transports.
* `PlainTransport`: Add support for SRTP.
* Rename `PlainRtpTransport` to `PlainTransport` everywhere (classes, methods, TypeScript types, etc). Keep previous names and mark them as DEPRECATED.
* Fix vulnarability in IPv6 parser.


### 3.4.13

* Update `uuid` dep to 7.0.X (new API).
* Fix crash due wrong array index in `PipeConsumer::FillJson()`.
  - Fixes #364


### 3.4.12

* TypeScript: generate `es2020` instead of `es6`.
* Update `usrsctp` library.
  - Fixes #362 (thanks @chvarlam for reporting it).


### 3.4.11

* `IceServer.cpp`: Reject received STUN Binding request with 487 if remote peer indicates ICE-CONTROLLED into it.


### 3.4.10

* `ProducerOptions`: Rename `keyFrameWaitTime` option to `keyFrameRequestDelay` and make it work as expected.


### 3.4.9

* Add `Utils::Json::IsPositiveInteger()` to not rely on `is_number_unsigned()` of json lib, which is unreliable due to its design.
* Avoid ES6 `export default` and always use named `export`.
* `router.pipeToRouter()`: Ensure a single `PipeTransport` pair is created between `router1` and `router2`.
   - Since the operation is async, it may happen that two simultaneous calls to `router1.pipeToRouter({ producerId: xxx, router: router2 })` would end up generating two pairs of `PipeTranports`. To prevent that, let's use an async queue.
* Add `keyFrameWaitTime` option to `ProducerOptions`.
* Update Node and C++ deps.


### 3.4.8

* `libsrtp.gyp`: Fix regression in mediasoup for Windows.
  - `libsrtp.gyp`: Modernize it based on the new `BUILD.gn` in Chromium.
  - `libsrtp.gyp`: Don't include "test" and other targets.
  - Assume `HAVE_INTTYPES_H`, `HAVE_INT8_T`, etc. in Windows.
  - Issue details: https://github.com/sctplab/usrsctp/issues/353
* `gyp` dependency: Add support for Microsoft Visual Studio 2019.
  - Modify our own `gyp` sources to fix the issue.
  - CL uploaded to GYP project with the fix.
  - Issue details: https://github.com/sctplab/usrsctp/issues/347


### 3.4.7

* `PortManager.cpp`: Do not limit the number of failed `bind()` attempts to 20 since it does not work well in scenarios that launch tons of `Workers` with same port range. Instead iterate all ports in the range given to the Worker.
* Do not copy `catch.hpp` into `test/include/` but make the GYP `mediasoup-worker-test` target include the corresponding folder in `deps/catch`.


### 3.4.6

* Update libsrtp to 2.3.0.
* Update ESLint and TypeScript deps.


### 3.4.5

* Update deps.
* Fix text in `./github/Bug_Report.md` so it no longer references the deprecated mailing list.


### 3.4.4

* `Transport.cpp`: Ignore RTCP SDES packets (we don't do anything with them anyway).
* `Producer` and `Consumer` stats: Always show `roundTripTime` (even if calculated value is 0) after a `roundTripTime` > 0 has been seen.


### 3.4.3

* `Transport.cpp`: Fix RTCP FIR processing:
  - Instead of looking at the media ssrc in the common header, iterate FIR items and look for associated `Consumers` based on ssrcs in each FIR item.
  - Fixes #350 (thanks @j1elo for reporting and documenting the issue).


### 3.4.2

* `SctpAssociation.cpp`: Improve/fix logs.
* Improve Node `EventEmitter` events inline documentation.
* `test-node-sctp.js`: Wait for SCTP association to be open before sending data.


### 3.4.1

* Improve mediasoup-worker build system by using `sh` instead of `bash` and default to 4 cores (thanks @smoke, PR #349).


### 3.4.0

* Add `worker.getResourceUsage()` API.
* Update OpenSSL to 1.1.1d.
* Update libuv to 1.34.0.
* Update TypeScript and ESLint NPM dependencies.


### 3.3.8

* Update usrsctp dependency (it fixes a potential wrong memory access).
  - More details in the reported issue: https://github.com/sctplab/usrsctp/issues/408


### 3.3.7

* Fix `version` getter.


### 3.3.6

* `SctpAssociation.cpp`: Initialize the `usrsctp` socket in the class constructor. Fixes #348.


### 3.3.5

* Fix usage of a deallocated `RTC::TcpConnection` instance under heavy CPU usage due to mediasoup deleting the instance in the middle of a receiving iteration. Fixes #333.
  - More details in the commit: https://github.com/versatica/mediasoup/commit/49824baf102ab6d2b01e5bca565c29b8ac0fec22


### 3.3.4

* IPv6 fix: Use `INET6_ADDRSTRLEN` instead of `INET_ADDRSTRLEN`.


### 3.3.3

* Add `consumer.setPriority()` and `consumer.priority` API to prioritize how the estimated outgoing bitrate in a transport is distributed among all video consumers (in case there is not enough bitrate to satisfy them).
* Make video `SimpleConsumers` play the BWE game by helping in probation generation and bitrate distribution.
* Add `consumer.preferredLayers` getter.
* Rename `enablePacketEvent()` and "packet" event to `enableTraceEvent()` and "trace" event (sorry SEMVER).
* Transport: Add a new "trace" event of type "bwe" with detailed information about bitrates.


### 3.3.2

* Improve "packet" event by not firing both "keyframe" and "rtp" types for the same RTP packet.


### 3.3.1


* Add type "keyframe" as a valid type for "packet" event in `Producers` and `Consumers`.


### 3.3.0

* Add transport-cc bandwidth estimation and congestion control in sender and receiver side.
* Run in Windows.
* Rewrite to TypeScript.
* Tons of improvements.


### 3.2.5

* Fix TCP leak (#325).


### 3.2.4

* `PlainRtpTransport`: Fix comedia mode.


### 3.2.3

* `RateCalculator`: improve efficiency in `GetRate()` method (#324).


### 3.2.2

* `RtpDataCounter`: use window size of 2500 ms instead of 1000 ms.
  - Fixes false "lack of RTP" detection in some screen sharing usages with simulcast.
  - Fixes #312.


### 3.2.1

* Add RTCP Extended Reports for RTT calculation on receiver RTP stream (thanks @yangjinechofor for initial pull request #314).
* Make mediasoup-worker compile in Armbian Debian Buster (thanks @krishisola, fixes #321).


### 3.2.0

* Add DataChannel support via DataProducers and DataConsumers (#10).
* SRTP: Add support for AEAD GCM (#320).


### 3.1.7

* `PipeConsumer.cpp`: Fix RTCP generation (thanks @vpalmisano).


### 3.1.6

* VP8 and H264: Fix regression in 3.1.5 that produces lot of changes in current temporal layer detection.


### 3.1.5

* VP8 and H264: Allow packets without temporal layer information even if N temporal layers were announced.


### 3.1.4

* Add `-fPIC` in `cflags` to compile in x86-64. Fixes #315.


### 3.1.3

* Set the sender SSRC on PLI and FIR requests [related thread](https://mediasoup.discourse.group/t/broadcasting-a-vp8-rtp-stream-from-gstreamer/93).


### 3.1.2

* Workaround to detect H264 key frames when Chrome uses external encoder (related [issue](https://bugs.chromium.org/p/webrtc/issues/detail?id=10746)). Fixes #313.


### 3.1.1

* Improve `GetBitratePriority()` method in `SimulcastConsumer` and `SvcConsumer` by checking the total bitrate of all temporal layers in a given producer stream or spatial layer.


### 3.1.0

* Add SVC support. It includes VP9 full SVC and VP9 K-SVC as implemented by libwebrtc.
* Prefer Python 2 (if available) over Python 3. This is because there are yet pending issues with gyp + Python 3.


### 3.0.12

* Do not require Python 2 to compile mediasoup worker (#207). Both Python 2 and 3 can now be used.


### 3.0.11

* Codecs: Improve temporal layer switching in VP8 and H264.
* Skip worker compilation if `MEDIASOUP_WORKER_BIN` environment variable is given (#309). This makes it possible to install mediasoup in platforms in which, somehow, gcc > 4.8 is not available during `npm install mediasoup` but it's available later.
* Fix `RtpStreamRecv::TransmissionCounter::GetBitrate()`.


### 3.0.10

* `parseScalabilityMode()`: allow "S" as spatial layer (and not just "L"). "L" means "dependent spatial layer" while "S" means "independent spatial layer", which is used in K-SVC (VP9, AV1, etc).


### 3.0.9

* `RtpStreamSend::ReceiveRtcpReceiverReport()`: improve `rtt` calculation if no Sender Report info is reported in received Received Report.
* Update libuv to version 1.29.1.


### 3.0.8

* VP8 & H264: Improve temporal layer switching.


### 3.0.7

* RTP frame-marking: Add some missing checks.


### 3.0.6

* Fix regression in proxied RTP header extensions.


### 3.0.5

* Add support for frame-marking RTP extensions and use it to enable temporal layers switching in H264 codec (#305).


### 3.0.4

* Improve RTP probation for simulcast/svc consumers by using proper RTP retransmission with increasing sequence number.


### 3.0.3

* Simulcast: Improve timestamps extra offset handling by having a map of extra offsets indexed by received timestamps. This helps in case of packet retransmission.


### 3.0.2

* Simulcast: proper RTP stream switching by rewriting packet timestamp with a new timestamp calculated from the SenderReports' NTP relationship. 


### 3.0.1

* Fix crash in `SimulcastConsumer::IncreaseLayer()` with Safari and H264 (#300).


### 3.0.0

* v3 is here!


### 2.6.19

* `RtpStreamSend.cpp`: Fix a crash in `StorePacket()` when it receives an old packet and there is no space left in the storage buffer (thanks to zkfun for reporting it and providing us with the solution).
* Update deps.


### 2.6.18

* Fix usage of a deallocated `RTC::TcpConnection` instance under heavy CPU usage due to mediasoup deleting the instance in the middle of a receiving iteration. 


### 2.6.17

* Improve build system by using all available CPU cores in parallel.


### 2.6.16

* Don't mandate server port range to be >= 99.


### 2.6.15

* Fix NACK retransmissions.


### 2.6.14

* Fix TCP leak (#325).


### 2.6.13

* Make mediasoup-worker compile in Armbian Debian Buster (thanks @krishisola, fixes #321).
* Update deps.


### 2.6.12

* Fix RTCP Receiver Report handling.


### 2.6.11

* Update deps.
* Simulcast: Increase profiles one by one unless explicitly forced (fixes #188).


### 2.6.10

* `PlainRtpTransport.js`: Add missing methods and events.


### 2.6.9

* Remove a potential crash if a single `encoding` is given in the Producer `rtpParameters` and it has a `profile` value.


### 2.6.8

* C++: Verify in libuv static callbacks that the associated C++ instance has not been deallocated (thanks @artushin and @mariat-atg for reporting and providing valuable help in #258).


### 2.6.7

* Fix wrong destruction of Transports in Router.cpp that generates 100% CPU usage in mediasoup-worker processes.


### 2.6.6

* Fix a port leak when a WebRtcTransport is remotely closed due to a DTLS close alert (thanks @artushin for reporting it in #259).


### 2.6.5

* RtpPacket: Fix Two-Byte header extensions parsing.


### 2.6.4

* Upgrade again to OpenSSL 1.1.0j (20 Nov 2018) after adding a workaround for issue [#257](https://github.com/versatica/mediasoup/issues/257).


### 2.6.3

* Downgrade OpenSSL to version 1.1.0h (27 Mar 2018) until issue [#257](https://github.com/versatica/mediasoup/issues/257) is fixed.


### 2.6.2

* C++: Remove all `Destroy()` class methods and no longer do `delete this`.
* Update libuv to 1.24.1.
* Update OpenSSL to 1.1.0g.


### 2.6.1

* worker: Internal refactor and code cleanup.
* Remove announced support for certain RTCP feedback types that mediasoup does nothing with (and avoid forwarding them to the remote RTP sender).
* fuzzer: fix some wrong memory access in `RtpPacket::Dump()` and `StunMessage::Dump()` (just used during development).

### 2.6.0

* Integrate [libFuzzer](http://llvm.org/docs/LibFuzzer.html) into mediasoup (documentation in the `doc` folder). Extensive testing done. Several heap-buffer-overflow and memory leaks fixed.


### 2.5.6

* `Producer.cpp`: Remove `UpdateRtpParameters()`. It was broken since Consumers
  were not notified about profile removed and so on, so they may crash.
* `Producer.cpp: Remove some maps and simplify streams handling by having a
  single `mapSsrcRtpStreamInfo`. Just keep `mapActiveProfiles` because
  `GetActiveProfiles()` method needs it.
* `Producer::MayNeedNewStream()`: Ignore new media streams with new SSRC if
  its RID is already in use by other media stream (fixes #235).
* Fix a bad memory access when using two byte RTP header extensions.


### 2.5.5

* `Server.js`: If a worker crashes make sure `_latestWorkerIdx` becomes 0.


### 2.5.4

* `server.Room()`: Assign workers incrementally or explicitly via new `workerIdx` argument.
* Add `server.numWorkers` getter.


### 2.5.3

* Don't announce `muxId` nor RTP MID extension support in `Consumer` RTP parameters.


### 2.5.2

* Enable RTP MID extension again.


### 2.5.1

* Disable RTP MID extension until [#230](https://github.com/versatica/mediasoup/issues/230) is fixed.


### 2.5.0

* Add RTP MID extension support.

### 2.4.6

* Do not close `Transport` on ICE disconnected (as it would prevent ICE restart on "recv" TCP transports).


### 2.4.5

* Improve codec matching.


### 2.4.4

* Fix audio codec matching when `channels` parameter is not given.


### 2.4.3

* Make `PlainRtpTransport` not leak if port allocation fails (related issue [#224](https://github.com/versatica/mediasoup/issues/224)).


### 2.4.2

* Fix a crash in when no more RTP ports were available (see related issue [#222](https://github.com/versatica/mediasoup/issues/222)).


### 2.4.1

* Update dependencies.


### 2.4.0

* Allow non WebRTC peers to create plain RTP transports (no ICE/DTLS/SRTP but just plain RTP and RTCP) for sending and receiving media.


### 2.3.3

* Fix C++ syntax to avoid an error when building the worker with clang 8.0.0 (OSX 10.11.6).


### 2.3.2

* `Channel.js`: Upgrade `REQUEST_TIMEOUT` to 20 seconds to avoid timeout errors when the Node or worker thread usage is too high (related to this [issue](https://github.com/versatica/mediasoup-client/issues/48)).


### 2.3.1

* H264: Check if there is room for the indicated NAL unit size (thanks @ggarber).
* H264: Code cleanup.


### 2.3.0

* Add new "spy" feature. A "spy" peer cannot produce media and is invisible for other peers in the room.


### 2.2.7

* Fix H264 simulcast by properly detecting when the profile switching should be done.
* Fix a crash in `Consumer::GetStats()` (see related issue [#196](https://github.com/versatica/mediasoup/issues/196)).


### 2.2.6

* Add H264 simulcast capability.


### 2.2.5

* Avoid calling deprecated (NOOP) `SSL_CTX_set_ecdh_auto()` function in OpenSSL >= 1.1.0.


### 2.2.4

* [Fix #4](https://github.com/versatica/mediasoup/issues/4): Avoid DTLS handshake fragmentation.


### 2.2.3

* [Fix #196](https://github.com/versatica/mediasoup/issues/196): Crash in `Consumer::getStats()` due to wrong `targetProfile`.


### 2.2.2

* Improve [issue #209](https://github.com/versatica/mediasoup/issues/209).


### 2.2.1

* [Fix #209](https://github.com/versatica/mediasoup/issues/209): `DtlsTransport`: don't crash when signaled fingerprint and DTLS fingerprint do not match (thanks @yangjinecho for reporting it).


### 2.2.0

* Update Node and C/C++ dependencies.


### 2.1.0

* Add `localIP` option for `room.createRtpStreamer()` and `transport.startMirroring()` [PR #199](https://github.com/versatica/mediasoup/pull/199).


### 2.0.16

* Improve C++ usage (remove "warning: missing initializer for member" [-Wmissing-field-initializers]).
* Update Travis-CI settings.


### 2.0.15

* Make `PlainRtpTransport` also send RTCP SR/RR reports (thanks @artushin for reporting).


### 2.0.14

* [Fix #193](https://github.com/versatica/mediasoup/issues/193): `preferTcp` not honored (thanks @artushin).


### 2.0.13

* Avoid crash when no remote IP/port is given.


### 2.0.12

* Add `handled` and `unhandled` events to `Consumer`.


### 2.0.11

* [Fix #185](https://github.com/versatica/mediasoup/issues/185): Consumer: initialize effective profile to 'NONE' (thanks @artushin).
* [Fix #186](https://github.com/versatica/mediasoup/issues/186): NackGenerator code being executed after instance deletion (thanks @baiyufei).


### 2.0.10

* [Fix #183](https://github.com/versatica/mediasoup/issues/183): Always reset the effective `Consumer` profile when removed (thanks @thehappycoder).


### 2.0.9

* Make ICE+DTLS more flexible by allowing sending DTLS handshake when ICE is just connected.


### 2.0.8

* Disable stats periodic retrieval also on remote closure of `Producer` and `WebRtcTransport`.


### 2.0.7

* [Fix #180](https://github.com/versatica/mediasoup/issues/180): Added missing include `cmath` so that `std::round` can be used (thanks @jacobEAdamson).


### 2.0.6

* [Fix #173](https://github.com/versatica/mediasoup/issues/173): Avoid buffer overflow in `()` (thanks @lightmare).
* Improve stream layers management in `Consumer` by using the new `RtpMonitor` class.


### 2.0.5

* [Fix #164](https://github.com/versatica/mediasoup/issues/164): Sometimes video freezes forever (no RTP received in browser at all).
* [Fix #160](https://github.com/versatica/mediasoup/issues/160): Assert error in `RTC::Consumer::GetStats()`.


### 2.0.4

* [Fix #159](https://github.com/versatica/mediasoup/issues/159): Don’t rely on VP8 payload descriptor flags to assure the existence of data.
* [Fix #160](https://github.com/versatica/mediasoup/issues/160): Reset `targetProfile` when the corresponding profile is removed.


### 2.0.3

* worker: Fix crash when VP8 payload has no `PictureId`.


### 2.0.2

* worker: Remove wrong `assert` on `Producer::DeactivateStreamProfiles()`.


### 2.0.1

* Update README file.


### 2.0.0

* New design based on `Producers` and `Consumer` plus a mediasoup protocol and the **mediasoup-client** client side SDK.


### 1.2.8

* Fix a crash due to RTX packet processing while the associated `NackGenerator` is not yet created.


### 1.2.7

* Habemus RTX ([RFC 4588](https://tools.ietf.org/html/rfc4588)) for proper RTP retransmission.


### 1.2.6

* Fix an issue in `buffer.toString()` that makes mediasoup fail in Node 8.
* Update libuv to version 1.12.0.


### 1.2.5

* Add support for [ICE renomination](https://tools.ietf.org/html/draft-thatcher-ice-renomination).


### 1.2.4

* Fix a SDP negotiation issue when the remote peer does not have compatible codecs.


### 1.2.3

* Add video codecs supported by Microsoft Edge.


### 1.2.2

* `RtpReceiver`: generate RTCP PLI when "rtpraw" or "rtpobject" event listener is set.


### 1.2.1

* `RtpReceiver`: fix an error producing packets when "rtpobject" event is set.


### 1.2.0

* `RtpSender`: allow `disable()`/`enable()` without forcing SDP renegotiation (#114).


### 1.1.0

* Add `Room.on('audiolevels')` event.


### 1.0.2

* Set a maximum value of 1500 bytes for packet storage in `RtpStreamSend`.


### 1.0.1

* Avoid possible segfault if `RemoteBitrateEstimator` generates a bandwidth estimation with zero SSRCs.


### 1.0.0

* First stable release.
