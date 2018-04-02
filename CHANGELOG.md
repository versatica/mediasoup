# Changelog


### 2.0.16

* Improve C++ usage (remove "warning: missing initializer for member" [-Wmissing-field-initializers]).
* Update Travis-CI settings.


### 2.0.15

* Make `PlainRtpTransport` also send RTCP SR/RR reports (thanks @artushin for reporting).


### 2.0.14

* [Fix #193](https://github.com/versatica/mediasoup/issues/193) `preferTcp` not honored (thanks @artushin).

### 2.0.13

* Avoid crash when no remote IP/port is given.


### 2.0.12

* Add `handled` and `unhandled` events to `Consumer`.


### 2.0.11

* [Fix #185](https://github.com/versatica/mediasoup/issues/185) Consumer: initialize effective profile to 'NONE' (thanks @artushin).
* [Fix #186](https://github.com/versatica/mediasoup/issues/186) NackGenerator code being executed after instance deletion (thanks @baiyufei).


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
