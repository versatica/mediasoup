# Changelog


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
