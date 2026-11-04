# TODO NEW RTCP

- Delete this file `TODO_NEW_RTCP.md`.

- Address all "TODO" comments in all `NEW_RTCP/` folders.

- Delete old RTCP classes, tests and fuzzers.

- Remove `NEW_` prefix everywhere in folder names (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in C++ `NEW_RTCP` namespace.

- Remove `NEW_` prefix in `#ifndef` in `.hpp` files (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in all `#define MS_CLASS` in `.cpp` files.

## During the migration to the new RTCP

- In `TransportWideCcFeedbackGenerator.cpp` add `static constexpr uint16_t MaxMissingRtpPackets{ 0x7FFE }` (libwebrtc's `kMaxMissingSequenceNumbers`) and use it in `FillAndSendFeedback()` to compute `oldestReportableSequenceNumber`.
