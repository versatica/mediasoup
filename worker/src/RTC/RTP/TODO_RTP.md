# TODO RTP

- In `RTP/SharedPacket` we clone the `Packet` but not sure about the buffer size since we don't limit it on reception. So `SharedPacket` constructor and `Assign()` methods can now throw and the caller must be ready. This is, in `Router` and `RtpRetransmissionBuffer`.

- Delete `RTP/SharedRtpPacket`.
