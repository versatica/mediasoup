# TODO new RTP Packet class

- Must override `Serialize()` to take care of DD pointers (if any).

- Same in `Clone()`.

- Specific Extensions getters/setters.

- Specific Extensions in `Dump()`.

- RTX and `ShiftPayload()` stuff.

- Spatial/temporal layers and `IsKeyframe()` stuff.

- DD stuff.

- `GetNextMediasoupPacketId()` static method.

- `MS_RTC_LOGGER_RTP` and `this->logger` stuff.
