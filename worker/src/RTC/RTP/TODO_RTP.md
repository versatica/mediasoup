# TODO new RTP Packet class

- When to call `Freeze()`?

- Padding stuff. When writting a Packet we must write number of padding bytes in the last byte.

- `Packet::GetPayloadLength()` doesn't include padding bytes.

- Must override `Serialize()` to take care of extensions' and DD pointers, etc.

- Same in `Clone()`.

- Extensions stuff.

- RTX stuff.

- Spatial/temporal layers stuff.

- DD stuff.

- `GetNextMediasoupPacketId()` static method.

- `MS_RTC_LOGGER_RTP` and `this->logger` stuff.
