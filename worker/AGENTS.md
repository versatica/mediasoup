# AGENTS.md

These rules apply to every C++ source file and header of the worker, that is, the files matched by the `CLANG_FORMAT_PATHS` constant in `scripts/clang-scripts.mjs` (`src/`, `include/`, `test/`, `fuzzer/` and `mocks/`).

## Building

- Build the worker with `MEDIASOUP_LOCAL_DEV=true make` and its tests with `MEDIASOUP_LOCAL_DEV=true make test`.
- Never use `npm install` / `npm ci` to build the worker: without `MEDIASOUP_LOCAL_DEV` their `postinstall` deletes the build directory, the downloaded subprojects and the local PIP/Meson/Ninja (`cleanWorkerArtifacts()` in `../npm-scripts.mjs`).

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

- New code avoids manual memory management: no `new`/`delete` on raw pointers. Whatever is allocated, or returned as an owning raw pointer by a factory (`Parse()`, `Clone()`, `Factory()`...), goes straight into a `std::unique_ptr` (`const` unless it is later `reset()`), created with `std::make_unique()` when it is our own allocation. This applies everywhere: classes, functions, tests and fuzzers.
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
- Standard headers already provided by `common.hpp` (`<algorithm>`, `<cinttypes>`, `<cstddef>`, `<cstdint>`, `<functional>`, `<memory>`, `<optional>`, `<utility>`) are never included again, neither in a `.cpp` nor in a `.hpp`. Others (`<vector>`, `<deque>`, `<map>`, `<string>`...) are included directly where used.
- A type newly used in a file gets its own `#include`. Never rely on transitive includes (except through the paired `.hpp`).
- Whenever code is touched, check the includes of the file: none missing and none left over.
- A standard header that provides functions or utilities rather than a type of its own name (`<cstring>`, `<cstdlib>`, `<cmath>`, `<limits>`, `<sstream>`...) carries a comment listing what is used from it, with `()` for functions: `#include <cstring> // std::memcmp(), std::memcpy()`. Headers named after the type they provide (`<vector>`, `<string>`, `<map>`...) take no comment.

## Class layout

- Class constants (`static constexpr` / `static const` members) go at the very top of the class body. Nested types (`struct`, `class`, `enum`, `using`) go right after them, before any method, under their own access specifier. A class constant whose type uses a nested type goes after that nested type. Lookup tables (`static const` maps such as `ChunkType2String`) are not class constants for this rule: they go in their own `private:` block right after the public static methods.
- A block of `static constexpr` / `static const` members always starts with its own `public:` or `private:` line, even when the previous block has the same access.
- Struct fields that need documentation use multiline `/** ... */` comments, never `//`.
- Public methods and class constants are documented with `/** ... */`. Parameter docs take a dash: `@param nowMs - Current time.`
- `@remarks` goes alone on its line (no colon, no text after it), and its content goes on the following lines as dash items (`- `), with continuation lines indented two spaces.

## Functions and statements

- Every function and method defined in a `.cpp` starts with `MS_TRACE()` (`MS_TRACE_STD()` in the `Channel` classes and other code that uses the `_STD` variants). This does not apply to the fuzzers (`fuzzer/`).
- Local declarations inside a function or at file scope: `constexpr` first, then `const`, then non-const, with a blank line between the `const` and the non-const groups. That order is not mandatory, though: logical order wins. The "parent" is declared first and then the "children" that are put into it or assigned to it, and each declaration stays next to the one it relates to. Making a line `const` never moves it: if it ends up next to a declaration of the other kind, a blank line goes between the two and nothing else.
- None of the above applies to the private/protected members of a class in the `.hpp`: there members go in the order that makes sense to whoever reads the class, grouped by what they are under their group comment, and a `const` member stays in its place with `const` added and nothing else. It is not moved up, not separated with a blank line and not taken out of its group.

  ```cpp
  	private:
  		// Bounds in force.
  		int64_t minBitrate{ 0 };
  		const int64_t startBitrate;
  		int64_t maxBitrate{ Types::BitrateInfinite };
  ```

- Code is split into paragraphs with blank lines, one per step: a declaration is not glued to the first thing done with it, an action is not glued to the assert/`REQUIRE` that checks it, and a check is not glued to the next action. What belongs to the same step stays together: several consecutive calls on the same object, the assignments that fill its fields, or several consecutive `REQUIRE`s.
- A declaration that carries its own comment is a paragraph: it is separated by a blank line from the next commented declaration.
- Everything that can be `const` is `const`, including `const auto` locals and `const T&` parameters. But never replace an explicit type with `auto` when the right-hand side does not show the type. Exception: iterators and similar types whose spelled-out name adds nothing for the reader can be `auto` (`auto chunkIt = packet->ChunksBegin()`, `auto it = map.find(key)`, `auto [it, inserted] = map.emplace(...)`).
- Value getters are named `GetXxx()`. Boolean getters are named `IsXxx()` / `HasXxx()`.
- Never `&foo` to take an address: use `std::addressof(foo)`.
- Never reach through an iterator (`it->second.field`): bind a named reference or copy first.
- Never use `.first` / `.second` inline: bind them to a named variable first (`const bool inserted = set.insert(x).second;`).
- Aggregates are built with designated initializers: `{ .ssrc = ssrc, .lastSeq = seq }`, never `{ ssrc, seq }` (clang-tidy `modernize-use-designated-initializers`).
- Never write the `struct` keyword before a variable, parameter or return type: `RttBackoff rttBackoff;`, never `struct RttBackoff rttBackoff;`. The only exception is where a function with the same name hides the type, as with `struct stat` and `struct sigaction` (POSIX declares the `stat()` and `sigaction()` functions, and without the keyword it does not compile). There it stays, with a one-line comment saying why, so that it is not "fixed" in a later pass.
- Every lambda states its return type explicitly, `-> void` included. Hence a lambda never takes `auto` parameters (clang-tidy `fuchsia-trailing-return` rejects a trailing return type on a generic lambda): spell out the parameter type, e.g. `const std::pair<const uint16_t, OutgoingStream>& entry` for a `std::map` entry.
- Integer literals have no `u` suffix (`0`, not `0u`), unless the value does not fit in an `int`.
- Write `Utils::Foo`, never `::Utils::Foo`.
- Access `std::optional` with `.value()`, not `*opt`.
- In production code, before touching a `.value()`, read upwards until finding what guarantees the value. If an `if`, an `else`, an early `return` or an earlier assignment guarantees it, it is left exactly as it is: no `NOLINT`, no local variable pulled out, no restructuring of the `if`, and no mention of it. Only accesses that are really unguarded are acted upon. The `NOLINT` rule for optionals is a test rule (see "Tests (Catch2)").

  ```cpp
  if (!this->avgRttUs.has_value())
  {
  	avgRttUs = windowRttUs;
  }
  else
  {
  	// The condition already guarantees a value here. No NOLINT, no local
  	// variable, no restructuring.
  	avgRttUs = this->avgRttUs.value() * 0.7 + windowRttUs * 0.3;
  }
  ```

- A function whose result the caller must check (e.g. a `bool` telling whether it did its job) is declared `[[nodiscard]]`, so that ignoring it fails at compile time instead of relying on the caller to remember.
- Use the `std::ranges` version of an algorithm whenever there is one, passing the container itself when the algorithm covers all of it (`std::ranges::fill(container, value)`, not `std::fill(std::begin(container), std::end(container), value)`).

## Robustness

- No input coming from the API or the network may crash the worker. `MS_ASSERT` aborts in release builds too, so it is only for invariants that hold by construction. Inputs are validated at the boundary and rejected with `MS_THROW_TYPE_ERROR`.

## Comments and logs

- All code text is in English.
- A comment must help whoever reads the file cold. Do not write comments that only record a discussion, and do not reference other projects the code may have been ported from.
- Never turn a point raised while working on a change into a comment: why a header was chosen over another, why something is measured one way and not another, what an earlier version did wrong, what a lint check would say. If the code is right, it needs no note defending it.
- No semicolons in log messages or comments: use a comma in logs and split into sentences in comments.
- Use ASCII `->`, never the Unicode arrow.
- Prefer `MS_WARN_TAG()` / `MS_WARN_2TAGS()` over `MS_DEBUG_DEV()` for conditions worth seeing in production.

## Tests (Catch2)

- Helpers are lambdas defined inside the `SCENARIO` body, never file-level `static` functions.
- Never `using namespace RTC;`: fully qualify names.
- A `SCENARIO` or `SECTION` name is a single string literal, never split into adjacent literals.
- Accessing a `std::optional` after `REQUIRE(opt.has_value())` needs `// NOLINTNEXTLINE(bugprone-unchecked-optional-access)`. The check flags all three forms, `*opt`, `opt->field` and `opt.value()`: using `.value()` does not avoid it, and the `REQUIRE()` above does not help because the checker does not see through the Catch2 macro.
- When there are several accesses to the same optional, bind the value once with a single `NOLINT` instead of repeating it per access, the same way as with iterators:

  ```cpp
  REQUIRE(loss.has_value());

  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto& lossValue = loss.value();

  REQUIRE(lossValue.expectedPackets == 100);
  ```

- Before calling a test file done, search it for `.value()`, `->` and `*` on optionals and check one by one that each access has its `NOLINT` or comes from an already bound value.
- Use the existing mocks (`mocks::MockShared`, `mocks::MockTimerHandle`, `mocks::MockBackoffTimerHandle`) instead of writing new ones.
- A test expectation states what the code must do. Never change an expected value, widen a tolerance or loosen an assertion just because the code now produces something else.
