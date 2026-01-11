# TODO new RTP Packet class

- Adapt `SharedRtpPacket` and move it and all `RtpXxxx` classes to `RTC/RTP/` folder.

- Test specific Extensions and payload descriptor stuff.

- In `Producer::ManglePacket()` do not assign Extension ids since `Packet::SetExtensions()` already does it.
