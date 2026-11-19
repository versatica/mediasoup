# AGENTS.md

These rules apply to every C++ source file and header of the worker, that is, the files matched by the `CLANG_FORMAT_PATHS` constant in `scripts/clang-scripts.mjs` (`src/`, `include/`, `test/`, `fuzzer/` and `mocks/`).

Every rule states a criterion. The cases, names and snippets that come with a rule illustrate that criterion, they never delimit it: a line that does not literally match any of them is judged by the criterion, never treated as uncovered because it is not listed. Only a list that a rule explicitly calls exhaustive (such as the headers provided by `common.hpp`) is closed.

## Building

- Build the worker with `MEDIASOUP_LOCAL_DEV=true make` and its tests with `MEDIASOUP_LOCAL_DEV=true make test`.
- Never use `npm install` / `npm ci` to build the worker: without `MEDIASOUP_LOCAL_DEV` their `postinstall` deletes the build directory, the downloaded subprojects and the local PIP/Meson/Ninja (`cleanWorkerArtifacts()` in `../npm-scripts.mjs`).

## Class members and constructors

- A member that the constructor must set gets no default initializer in the `.hpp`, so that the compiler complains if a constructor forgets it.
- A member whose value comes verbatim from a constructor argument is set in the member initializer list, never assigned in the constructor body.
- If every constructor is guaranteed to set a member, whether in the initializer list or in its body, the member gets no default initializer in the `.hpp`: the constructor always overwrites it, so a default would be dead code.
- A member that not every constructor sets (several constructors, or a default constructor) keeps its default initializer, unless its type already default-constructs to a defined value (a class or struct whose fields all have default initializers, `std::string`, containers, smart pointers...). Then no `{}` is added, since it would change nothing.
- A member whose initial value is a constant that does not depend on any argument gets that value as its initializer in the `.hpp`, not in the initializer list.
- A `std::optional` member that starts empty gets no initializer at all: `std::optional<uint16_t> frameNumber;`, never `{ std::nullopt }`, which is what it already is.
- The initializer list follows the declaration order of the members. Never reorder members to fit the initializer list.
- `explicit` goes only on constructors that can be called with a single argument (one parameter, or several with all but the first defaulted), never on one that takes none or that always takes several.

## `const` members

- Every member that is set in the constructor and never written again is `const`: `const T x` for values, `T* const x` for pointers (including `listener` and `shared`), `const std::unique_ptr<T>` for owned objects, `const std::function<...>` for callbacks.
- Before making a member `const`, check that it is not written from subclasses, not passed by non-const reference, and not mutated through non-const methods.
- Do not make members `const` in value types that are copied, assigned, swapped or stored in containers that need assignment (`std::vector` erase/insert, `ankerl::unordered_dense` maps, `std::optional` reassignment, etc.), nor in wire-format structs (RTCP, SCTP, STUN headers).
- Making a member `const` never moves it: it stays where it was in the class, with `const` added and nothing else.

## Ownership

- New code avoids manual memory management: no `new`/`delete` on raw pointers. Whatever is allocated, or returned as an owning raw pointer by a factory (`Parse()`, `Clone()`, `Factory()`...), goes straight into a `std::unique_ptr` (`const` unless it is later `reset()`), created with `std::make_unique()` when it is our own allocation. This applies everywhere: classes, functions, tests and fuzzers.
- A `Xxx::Parse()` or `Xxx::Factory()` call that is expected to fail and return `nullptr` is checked directly, never stored in a `std::unique_ptr`, since nothing was allocated: `REQUIRE(!RTC::SCTP::SackChunk::Parse(buffer, sizeof(buffer)));`.
- An object owned by a class is held in a `std::unique_ptr`, never as a raw pointer with `new`/`delete`.
- If it is created once in the constructor and never replaced, the `std::unique_ptr` is `const` and is created in the initializer list (a ternary is fine when it is optional and the condition is known at construction).
- If it is created lazily, conditionally with complex logic, or replaced during the object's life, the `std::unique_ptr` is not `const` and is set with `reset()` or `std::make_unique()`.
- A destructor does not `clear()` containers nor `reset()` smart pointers that the destruction of the members empties or destroys right afterwards anyway. Before removing one, follow every chain of calls that destroying that member starts, to its end: the destructors of the owned objects, of their own members, and everything any of them triggers (calls to listeners or to any other object, callbacks, closed handles), and then whatever those calls trigger in turn. Every object that any link of a chain touches must still be alive at the moment the chain runs, whether it is the owner, another member of the owner, an object reached through one of them or a third party. Moving a destruction changes that moment:
  - Implicit destruction happens after the destructor body, in reverse declaration order, so by then everything the body deleted and every member declared after this one is already gone. In a base class, the subclass is gone too.
  - `std::unique_ptr::reset()` nulls the pointer before destroying the object, unlike `delete` followed by `= nullptr`, so a chain that reads that same pointer finds it null.
  - Example: in `WebRtcTransport`, destroying the ICE server notifies the removal of every tuple, the transport then closes the TCP connection of each one, and those connections belong to TCP servers that the destructor body deletes. Destroying the ICE server implicitly, after the body, would make that chain touch deleted TCP connections, even though nothing in it calls the destroyed member back.
  - A callback that libuv defers to a later loop iteration (`uv_close()`, a pending send) is not part of the chain, but whatever it touches when it runs must not depend on the destroyed objects either.
- Constructors have side effects too: they may register something that must be unique while it lives (a timer label, a channel handler id, a bound port). So replacing an owned object is not order free. Assigning a new `std::unique_ptr` over an existing one (`ptr = std::make_unique<T>(...)`, `map[key] = std::make_unique<T>(...)`) constructs the new object while the old one is still alive and destroys the old one only afterwards, unlike `delete` of the old one followed by `new`. Whenever code is changed so that an object is created or destroyed at another moment, check what both its constructor and its destructor register, release or trigger, and keep the original order (for example `map.erase(key)` before creating the replacement) with a comment saying why.
- Where destruction order matters (for example, a DTLS transport that must go before the sockets, or an ICE server whose destructor closes TCP connections of servers deleted later in the same destructor), keep an explicit `reset()` in the destructor at the place where the `delete` would have been, with a comment saying why it must go there.
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
- The standard headers provided by `common.hpp`, which are exactly `<algorithm>`, `<cinttypes>`, `<cstddef>`, `<cstdint>`, `<functional>`, `<memory>`, `<optional>` and `<utility>`, are never included again, neither in a `.cpp` nor in a `.hpp`. Others (`<vector>`, `<deque>`, `<map>`, `<string>`...) are included directly where used.
- A type newly used in a file gets its own `#include`. Never rely on transitive includes (except through the paired `.hpp`).
- Whenever code is touched, check the includes of the file: none missing and none left over.
- A standard header that provides functions or utilities rather than a type of its own name (`<cstring>`, `<cstdlib>`, `<cmath>`, `<limits>`, `<sstream>`...) carries a comment listing what is used from it, with `()` for functions: `#include <cstring> // std::memcmp(), std::memcpy()`. Headers named after the type they provide (`<vector>`, `<string>`, `<map>`...) take no comment.

## Class layout

- Class constants (`static constexpr` members) go at the very top of the class body. Nested types (`struct`, `class`, `enum`, `using`) go right after them, before any method, under their own access specifier. A class constant whose type uses a nested type goes after that nested type.
- `static const` members (`Logger::Pid`, lookup tables such as `ChunkType2String`...) are not class constants for this rule: they go together with the rest of the static data members, in the block right after the static methods.
- A block of `static constexpr` members always starts with its own `public:` or `private:` line, even when the previous block has the same access.
- Struct fields that need documentation use multiline `/** ... */` comments, never `//`.
- Public methods and class constants are documented with `/** ... */`. Parameter docs take a dash: `@param nowMs - Current time.`
- `@remarks` goes alone on its line (no colon, no text after it), and its content goes on the following lines as dash items (`- `), with continuation lines indented two spaces.

## Functions and statements

- Every function and method defined in a `.cpp` starts with `MS_TRACE()` (`MS_TRACE_STD()` in the `Channel` classes and other code that uses the `_STD` variants). This does not apply to the fuzzers (`fuzzer/`).
- Local declarations inside a function or at file scope: `constexpr` first, then `const`, then non-const, with a blank line between the `const` and the non-const groups. A declaration that starts with `const` belongs to the `const` group, even if only its pointee is `const` (`const uint8_t* ptr`). This also applies when a line is made `const`: it moves up into the `const` group. The only exception is a real reason, logical or aesthetic, to declare a non-const first: the "parent" is declared first and then the "children" that are put into it or assigned to it, and a declaration that only makes sense next to the one it relates to stays there.
- None of the above applies to the private/protected members of a class in the `.hpp`: there members go in the order that makes sense to whoever reads the class, grouped by what they are under their group comment, and a `const` member stays in its place with `const` added and nothing else. It is not moved up, not separated with a blank line and not taken out of its group.

  ```cpp
  	private:
  		// Bounds in force.
  		int64_t minBitrate{ 0 };
  		const int64_t startBitrate;
  		int64_t maxBitrate{ Types::BitrateInfinite };
  ```

- Code is split into paragraphs with blank lines, one per step: a declaration is not glued to the first thing done with it, an action is not glued to the assert/`REQUIRE` that checks it, and a check is not glued to the next action. Whatever belongs to the same step stays together, for example several consecutive calls on the same object, the assignments that fill its fields, or several consecutive `REQUIRE`s.
- A declaration, `const` or not, is separated by a blank line from whatever is done with it afterwards, including the assignments that fill its fields: `Params params;`, blank line, then `params.ssrc = ...;`.
- A statement that leaves the block (`return`, `break`, `continue`, `throw`, and any macro that throws or leaves it, such as `MS_THROW_ERROR()`, `MS_THROW_TYPE_ERROR()`, `MS_THROW_NOT_FOUND_ERROR()` and their `_STD` variants) always has a blank line before it, unless it is the first statement of its block.
- A declaration that carries its own comment is a paragraph: it is separated by a blank line from the next commented declaration.
- Everything that can be `const` is `const`, including `const auto` locals and `const T&` parameters.
- A pointer parameter whose pointee the function does not modify is `const T*`. The pointer itself is never made `const` in a parameter (`T* const`): that `const` is not part of the signature and tells the caller nothing.
- In a listener callback, the pointer to the object emitting it, and to any entity that the listeners manage (`RTC::Producer`, `RTC::Consumer`, `RTC::TcpConnection`...), is never `const T*`: `OnDataProducerPaused(RTC::DataProducer* dataProducer)`, `OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer)`. The emitter cannot assume what the listener, usually its parent and owner, wants to do with it, which may be something non-const, such as closing the object because the event is wrong in its current state. The only exception is a callback that just queries the listener for a value (`OnDataConsumerNeedBufferedAmount()`) and is invoked from a const method of the emitter, which takes `const T*`.
- The other pointer parameters of a listener callback, data that the emitter owns or passes along (packets, addresses, keys...), are `const T*` whenever possible, since the listener is not expected to modify them: `OnProducerSendRtcpPacket(RTC::Producer* producer, const RTC::RTCP::Packet* packet)`.
- `auto` or an explicit type is decided by one criterion: can whoever reads the line tell the type from the line itself? If yes, `auto`. If not, spell the type out. The line shows the type when the right-hand side names it (a cast, `std::make_unique()`, a function or method whose name says what it returns), or when the name of the variable, read together with what it is assigned from, makes the type obvious: it is named after the member, field, container or entry it comes from, whatever word it adds before or after that name. These are descriptions of the criterion, not a closed list of allowed cases: any line where the type is evident qualifies, and a line that does not literally match one of them is judged by the criterion, never rejected for not matching. Iterators and similar types whose spelled-out name adds nothing for the reader are always `auto`.
- Class methods, static or not, and namespace functions always start with an uppercase letter, `constexpr` ones included: `GenerateCrcTables()`, never `generateCrcTables()`. Only file-scope `static` functions in a `.cpp` (libuv and OpenSSL callbacks and similar) start with a lowercase letter: `onCloseTcp()`.
- A method of the class itself is called without `this->`: `Send(...)`, never `this->Send(...)`. `this->` is only for members: `this->bufferBuilder`.
- A `static` constant of a class (`static constexpr` or `static const`) declared in its `.hpp` always carries the class prefix, in every mention, also inside the class's own methods: `StunPacket::FixedHeaderLength`, never `FixedHeaderLength`.
- Value getters are named `GetXxx()`. Boolean getters are named `IsXxx()` / `HasXxx()`.
- Never `&foo` to take an address: use `std::addressof(foo)`.
- Never reach through an iterator (`it->second.field`): bind a named reference or copy first.
- Never use `.first` / `.second` inline: bind them to a named variable first (`const bool inserted = set.insert(x).second;`).
- Use a structured binding whenever both members of a pair or tuple are used: `for (const auto& [key, portRange] : mapPortRanges)`, never `const auto& key = kv.first;` plus `const auto& portRange = kv.second;`.
- A loop over every element of a container is range-based: `for (const auto& codec : this->codecs)`. An index or an iterator stays only when the loop needs it: to index another container in parallel, to erase while iterating, to cover part of the range, or to use the position itself.
- Aggregates are built with designated initializers: `{ .ssrc = ssrc, .lastSeq = seq }`, never `{ ssrc, seq }` (clang-tidy `modernize-use-designated-initializers`).
- Never a trailing comma after the last element of a braced list (initializer lists, designated initializers, enums...): `{ 1, 2, 3 }`, never `{ 1, 2, 3, }`.
- Never write the `struct` keyword before a variable, parameter or return type: `RttBackoff rttBackoff;`, never `struct RttBackoff rttBackoff;`. The only exception is where a function with the same name hides the type, as with `struct stat` and `struct sigaction` (POSIX declares the `stat()` and `sigaction()` functions, and without the keyword it does not compile). There it stays, with a one-line comment saying why, so that it is not "fixed" in a later pass.
- Every lambda states its return type explicitly, `-> void` included. Hence a lambda never takes `auto` parameters (clang-tidy `fuchsia-trailing-return` rejects a trailing return type on a generic lambda): spell out the parameter type, e.g. `const std::pair<const uint16_t, OutgoingStream>& entry` for a `std::map` entry.
- Integer literals have no unsigned suffix (`0`, not `0u`), unless the suffix is needed: the value does not fit in an `int`, or the literal must not be an `int` (e.g. so that a template argument is deduced as an unsigned type). When needed, the suffix is uppercase: `4711U`, never `4711u`.
- Bitwise operations (`<<`, `>>`, `&`, `|`, `^`, `~`) work on unsigned operands, including operands that only become signed through integer promotion (a `uint8_t` or `uint16_t` shifted becomes an `int`). Make the operand unsigned at no runtime cost: `uint32_t{ byte } << 8`, `(uint32_t{ chunk } >> 13) & 0x03`, or test a bit with a mask instead of shifting it down (`(buffer[0] & 0x20) == 0`, not `((buffer[0] >> 5) & 0x01) == 0`). If the result must be signed, shift unsigned and convert the result: `static_cast<int64_t>(uint64_t{ 0x3FB93 } << 30)`. This applies even when the value is provably non-negative and there is no real bug: it is fixed this way, never silenced with `NOLINT(bugprone-signed-bitwise)`. The only exception is an operation that needs signed semantics on purpose (an arithmetic right shift of a possibly negative value), which keeps its `NOLINTNEXTLINE` with a comment saying why.
- A `NOLINT` only goes on code that is already correct, and that is proven before adding it. For `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`, find what bounds each index: a size check, an `MS_ASSERT`, a `REQUIRE()`, the loop condition, a `std::min()`. If nothing does (for example a test helper doing `items[0]` or `items.front()` without checking that `items` is not empty), the guard is missing and gets added (a `REQUIRE()` in tests; in production code, ask first), it is not silenced.
- `NOLINTNEXTLINE(<check>)` goes right above each physical line that is flagged, also inside a statement split across several lines. `NOLINTBEGIN(<check>)` / `NOLINTEND(<check>)` is only for a run of consecutive lines that all make the flagged access, such as a group of `REQUIRE()`s on `items[0]`, `items[1]`..., placed after the `REQUIRE(items.size() == N)` that guarantees them, with a blank line in between. It never wraps a loop, a whole block or lines that do something else.
- A `NOLINTNEXTLINE(<check>)` that goes right after a comment is separated from it by an empty `//` line:

  ```cpp
  // the first outstanding DATA chunk sent to that address."
  //
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  if (toBeSent[0].first == ...)
  ```
- Write `Utils::Foo`, never `::Utils::Foo`.
- Access `std::optional` with `.value()`, not `*opt`.
- In production code, before touching a `.value()`, read upwards until finding what guarantees the value. If anything earlier in the code path guarantees it (an `if`, an `else`, an early `return`, an earlier assignment or anything else), it is left exactly as it is: no `NOLINT`, no local variable pulled out, no restructuring of the `if`, and no mention of it. Only accesses that are really unguarded are acted upon. The `NOLINT` rule for optionals is a test rule (see "Tests (Catch2)").

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
- Never use `assert()`, always `MS_ASSERT()`. `assert()` disappears in release builds, so an invariant it checks is not checked in production.

## Comments and logs

- All code text is in English.
- A comment must help whoever reads the file cold. Do not write comments that only record a discussion, and do not add references to projects the code may have been ported from. References already in the code stay.
- Never turn a point raised while working on a change into a comment: why a header was chosen over another, why something is measured one way and not another, what an earlier version did wrong. If the code is right, it needs no note defending it.
- No semicolons in log messages or comments: use a comma in logs and split into sentences in comments.
- Use ASCII `->`, never the Unicode arrow.
- Prefer `MS_WARN_TAG()` / `MS_WARN_2TAGS()` over `MS_DEBUG_DEV()` for conditions worth seeing in production.

## Tests (Catch2)

- Everything a test needs is defined inside the `SCENARIO` body, never at file level, neither `static` nor inside an anonymous `namespace`: helpers are lambdas, mock listeners and other helper types are local classes, and constants are local `constexpr`. Something goes at file level only if several `SCENARIO`s in the same file share it.
- Inside the `SCENARIO` body, the local `constexpr` constants go first, then the local classes, then the objects and the helper lambdas, the same order as in a class body.
- A constant or a helper lambda that a local class uses is declared `static` and goes with the constants, before the classes: a local class cannot odr-use the automatic variables of the enclosing function. A constant is `static constexpr`, a lambda is `static const`. The same goes for a constant that a lambda without captures uses by reference (calling a method on it, passing it to a `const T&` parameter, comparing it inside a `REQUIRE()`, whose operands Catch2 takes by `const T&`): that odr-uses it, which a lambda can only do with a variable it captures or one that is `static`.
- A constant that a default argument names, of a lambda or of a method of a local class, is `static constexpr`. A plain `constexpr` local is an automatic variable, and GCC rejects it there (`local variable 'X' may not appear in this context`) even though clang accepts it, so a clean clang build proves nothing. A default argument never calls a local lambda, not even a `static` one: a constructor that needs one gets a default constructor that delegates to the one taking the argument.
- A lambda is never declared `constexpr`, anywhere in the worker: it is a function, not a compile-time value, and a lambda without captures is constant-initialized anyway. The only exception is a lambda that really is called in a constant expression (a `static_assert`, a template argument, the initializer of another `constexpr`).
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

- The check diagnoses the access itself, whatever is done with the result, and only a `has_value()` check with an early exit in the same function counts as a guard. So an access with no such guard always gets the `NOLINT`, even when the value is guaranteed by other means (a default member initializer, a `REQUIRE()`, the caller) and even when it is stored in a `const auto`:

  ```cpp
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();
  ```

- A clean `make tidy` run does not prove that an access needs no `NOLINT`: in very large functions (such as the `SCENARIO` of `TestAssociation.cpp`) the analysis gives up silently and reports nothing, and the warning appears as soon as the function shrinks. The only exception is an access inside a lambda, which the check never reports, so it takes no `NOLINT`.
- Before calling a test file done, search it for `.value()`, `->` and `*` on optionals and check one by one that each access has its `NOLINT` or comes from an already bound value.
- Use the existing mocks (`mocks::MockShared`, `mocks::MockTimerHandle`, `mocks::MockBackoffTimerHandle`) instead of writing new ones.
- A test expectation states what the code must do. Never change an expected value, widen a tolerance or loosen an assertion just because the code now produces something else.
