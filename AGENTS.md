# AGENTS.md

## How to work

- Never fix anything the quick way. Before changing code, silencing a warning (`NOLINT`, a cast, a config option) or proposing a change, investigate in depth why the problem happens, reason about it, and verify the conclusion against the code. Only then propose or apply the fix.
- A suppression is the last resort, used only once the cause is understood and it is proven that no real fix exists. If similar code does not trigger the same warning or problem, find the difference before doing anything: it is usually the real cause.
- Never propose something that has not been verified (types, callers, whether it can actually be implemented). A plausible guess presented as a conclusion is worse than saying it has not been checked yet.

## Worker (C++)

The rules for the C++ code of the mediasoup worker (its sources, headers, tests, mocks and fuzzers) live in [`worker/AGENTS.md`](worker/AGENTS.md). Read it before touching any file under `worker/`. They apply only to that C++ code, not to the Node.js or Rust code of this repository.
