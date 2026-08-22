# TODO NEW RTCP

- Remove `NEW_` prefix in folders.

- Remove `NEW_` prefix in C++ `NEW_RTCP` namespace.

- Remove `NEW_` prefix in `#ifndef` in `.hpp` files.

- We are not implementing `NeedsConsolidation()` in `Packet` class (only in `CompoundPacket` class). For now it's ok. Let's see.

- `Packet`: Check if we need all those `Get/SetVariableLengthXxxxxx()` methods.

- All the `TODO` comments.

- Tests for `Packet` and `CompoundPacket`.

- Fuzzer for `Packet` and `CompoundPacket`.
