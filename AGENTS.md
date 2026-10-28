# AGENTS.md

These rules apply to the C++ code under `worker/` (`include/`, `src/`, `mocks/`, `test/` and `fuzzer/`).

## Building

- Build the worker with `MEDIASOUP_LOCAL_DEV=true make -C worker` and its tests with `MEDIASOUP_LOCAL_DEV=true make -C worker test`.
- Never use `npm install` / `npm ci` to build the worker: without `MEDIASOUP_LOCAL_DEV` their `postinstall` deletes the build directory, the downloaded subprojects and the local PIP/Meson/Ninja (`cleanWorkerArtifacts()` in `npm-scripts.mjs`).

## Class members and constructors

- A member that the constructor must set gets no default initializer in the `.hpp`, so that the compiler complains if a constructor forgets it.
- A member whose value comes verbatim from a constructor argument is set in the member initializer list, never assigned in the constructor body.
- A member that the constructor normalizes, validates or computes before assigning keeps its default initializer in the `.hpp` and is assigned in the body.
- A member that not every constructor sets (several constructors, or a default constructor) keeps its default initializer.
- A member whose initial value is a constant that does not depend on any argument gets that value as its initializer in the `.hpp`, not in the initializer list.
- The initializer list follows the declaration order of the members. Never reorder members to fit the initializer list.

## `const` members

- Every member that is set in the constructor and never written again is `const`: `const T x` for values, `T* const x` for pointers (including `listener` and `shared`), `const std::unique_ptr<T>` for owned objects, `const std::function<...>` for callbacks.
- Before making a member `const`, check that it is not written from subclasses, not passed by non-const reference, and not mutated through non-const methods.
- Do not make members `const` in value types that are copied, assigned, swapped or stored in containers that need assignment (`std::vector` erase/insert, `ankerl::unordered_dense` maps, `std::optional` reassignment, etc.), nor in wire-format structs (RTCP, SCTP, STUN headers).
- Making a member `const` never moves it: it stays where it was in the class, with `const` added and nothing else.

## Ownership

- An object owned by a class is held in a `std::unique_ptr`, never as a raw pointer with `new`/`delete`.
- If it is created once in the constructor and never replaced, the `std::unique_ptr` is `const` and is created in the initializer list (a ternary is fine when it is optional and the condition is known at construction).
- If it is created lazily, conditionally with complex logic, or replaced during the object's life, the `std::unique_ptr` is not `const` and is set with `reset()` or `std::make_unique()`.
- Where destruction order matters (for example, a DTLS transport that must go before the sockets), keep an explicit `reset()` in the destructor at the place where the `delete` would have been.
- Exceptions: libuv handles (`uv_*_t`), which are freed in the `uv_close()` callback, and raw buffers created with `new[]`.

## Timers

- `TimerHandleInterface` and `BackoffTimerHandleInterface` instances are owned through `std::unique_ptr`, created with `shared->CreateTimer()` / `shared->CreateBackoffTimer()`, preferably in the initializer list.
- Compare the timer received in `OnTimer()` against `this->timer.get()`.
- Do not call `Stop()` on a timer right before destroying it: its destructor already closes it.
- Timer labels are derived, never invented: the file's class name in kebab-case, then the purpose taken from the member name without `Timer`/`Periodic` (`inactivityCheckPeriodicTimer` in `RtpStreamRecv` -> `"rtp-stream-recv-inactivity-check"`). A label never ends in `-timer`, `-timeout` or `-periodic`, except to tell apart two timers of the same class.

## Includes

- Every `.hpp` includes `"common.hpp"`.
- A `.cpp` includes `"common.hpp"` only if it has no paired `.hpp` (tests, `main.cpp`, fuzzers).
- A `.cpp` never re-includes a header that its paired `.hpp` already includes.
- Standard headers already provided by `common.hpp` (`<algorithm>`, `<cinttypes>`, `<cstddef>`, `<cstdint>`, `<functional>`, `<memory>`, `<optional>`, `<utility>`) are never included again. Others (`<vector>`, `<map>`, `<string>`...) are included directly where used.
- A type newly used in a file gets its own `#include`. Never rely on transitive includes (except through the paired `.hpp`).

## Class layout

- Nested types (`struct`, `class`, `enum`, `using`) go at the very top of the class body, before any method, under their own access specifier.
- A block of `static constexpr` / `static const` members always starts with its own `public:` or `private:` line, even when the previous block has the same access.
- Struct fields that need documentation use multiline `/** ... */` comments, never `//`.
- Public methods and class constants are documented with `/** ... */`. Parameter docs take a dash: `@param nowMs - Current time.`

## Functions and statements

- Every function and method defined in a `.cpp` starts with `MS_TRACE()` (`MS_TRACE_STD()` in the `Channel` classes and other code that uses the `_STD` variants).
- Local declarations inside a function: `constexpr` first, then `const`, then non-const, with a blank line between groups. This does NOT apply to the private/protected member declarations of a class: there members are grouped by meaning with their group comment.
- Code is split into paragraphs with blank lines: a declaration is not glued to the first thing done with it, an action is not glued to the assert/`REQUIRE` that checks it, and a check is not glued to the next action.
- Everything that can be `const` is `const`, including `const auto` locals and `const T&` parameters. But never replace an explicit type with `auto` when the right-hand side does not show the type.
- Value getters are named `GetXxx()`. Boolean getters are named `IsXxx()` / `HasXxx()`.
- Never `&foo` to take an address: use `std::addressof(foo)`.
- Never reach through an iterator (`it->second.field`): bind a named reference or copy first.
- Never use `.first` / `.second` inline: bind them to a named variable first (`const bool inserted = set.insert(x).second;`).
- Aggregates are built with designated initializers: `{ .ssrc = ssrc, .seq = seq }`.
- Never write the `struct` keyword before a variable, parameter or return type (`RttBackoff rttBackoff;`), except where a function hides the type (`struct stat`).
- Every lambda states its return type explicitly, `-> void` included.
- Integer literals have no `u` suffix (`0`, not `0u`), unless the value does not fit in an `int`.
- Write `Utils::Foo`, never `::Utils::Foo`.
- Access `std::optional` with `.value()`, not `*opt`. In production code, a `.value()` already guarded by the surrounding flow is left alone (no `NOLINT`).

## Robustness

- No input coming from the API or the network may crash the worker. `MS_ASSERT` aborts in release builds too, so it is only for invariants that hold by construction. Inputs are validated at the boundary and rejected with `MS_THROW_TYPE_ERROR`.

## Comments and logs

- All code text is in English.
- A comment must help whoever reads the file cold. Do not write comments that only record a discussion, and do not reference other projects the code may have been ported from.
- No semicolons in log messages or comments: use a comma in logs and split into sentences in comments.
- Use ASCII `->`, never the Unicode arrow.
- Prefer `MS_WARN_TAG()` / `MS_WARN_2TAGS()` over `MS_DEBUG_DEV()` for conditions worth seeing in production.

## Tests (Catch2)

- Helpers are lambdas defined inside the `SCENARIO` body, never file-level `static` functions.
- Never `using namespace RTC;`: fully qualify names.
- A `SCENARIO` or `SECTION` name is a single string literal, never split into adjacent literals.
- Accessing a `std::optional` after `REQUIRE(opt.has_value())` needs `// NOLINTNEXTLINE(bugprone-unchecked-optional-access)`. When there are several accesses, bind the value once (`const auto& value = opt.value();`) with a single `NOLINT`.
- Use the existing mocks (`mocks::MockShared`, `mocks::MockTimerHandle`, `mocks::MockBackoffTimerHandle`) instead of writing new ones.
- A test expectation states what the code must do. Never change an expected value, widen a tolerance or loosen an assertion just because the code now produces something else.
