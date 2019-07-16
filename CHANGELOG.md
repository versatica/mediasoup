# Changelog


### 3.2.1 (COMING SOON)

* Add RTCP Extended Reports for RTT calculation on receiver RTP stream (Thanks @yangjinechofor for initial pull request) .


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

* `server.Room()`: Assign workers incrementally or explicitely via new `workerIdx` argument.
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

* `Channel.js`: Upgrade `REQUEST_TIMEOUT` to 20 seconds to avoid timeout errors when the the Node or worker thread usage is too high (related to this [issue](https://github.com/versatica/mediasoup-client/issues/48)).


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
