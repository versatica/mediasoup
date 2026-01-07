# TODO new RTP Packet class

- Padding stuff. When writting a Packet we must write number of padding bytes in the last byte.

- `Packet::GetPayloadLength()` doesn't include padding bytes. It's ok.

- Must override `Serialize()` to take care of DD pointers (if any).

- Same in `Clone()`.

- Specific Extensions getters/setters.

- Specific Extensions in `Dump()`.

- RTX stuff.

- Spatial/temporal layers stuff.

- DD stuff.

- `GetNextMediasoupPacketId()` static method.

- `MS_RTC_LOGGER_RTP` and `this->logger` stuff.
