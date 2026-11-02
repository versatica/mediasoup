# TODO NEW RTCP

- Verify if libwebrtc implements RTCP packets/feedbacks that we don't and we should.

- In `Transport.cpp` we currently ignore received RTCP BYE packets. Should we handle them? Let's check libwebrtc and so on.

- Delete this file `TODO_NEW_RTCP.md`.

- Address all "TODO" comments in all `NEW_RTCP/` folders.

- Delete old RTCP classes, tests and fuzzers.

- Remove `NEW_` prefix everywhere in folder names (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in C++ `NEW_RTCP` namespace.

- Remove `NEW_` prefix in `#ifndef` in `.hpp` files (including `test/`, `fuzzer/`, etc).

- Remove `NEW_` prefix in all `#define MS_CLASS` in `.cpp` files.
