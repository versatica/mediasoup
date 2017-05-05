# Changelog


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
