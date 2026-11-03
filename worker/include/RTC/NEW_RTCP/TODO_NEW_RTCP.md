# TODO NEW RTCP

- Maybe handle received RTCP BYE. Currently we ignore them in `Transport.cpp`. In libwebrtc, `RTCPReceiver::HandleBye()` drops everything it keeps about that sender: the RTT samples, the report blocks received from that sender SSRC, the last FIR, its RRTR (XR) entry and the XR RTT. It never sends BYE.

- Delete this file `TODO_NEW_RTCP.md`.

- Address all "TODO" comments in all `NEW_RTCP/` folders.

- Delete old RTCP classes, tests and fuzzers.

- Remove `NEW_` prefix everywhere in folder names (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in C++ `NEW_RTCP` namespace.

- Remove `NEW_` prefix in `#ifndef` in `.hpp` files (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in all `#define MS_CLASS` in `.cpp` files.
