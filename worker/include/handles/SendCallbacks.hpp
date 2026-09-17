#ifndef MS_SEND_CALLBACKS_HPP
#define MS_SEND_CALLBACKS_HPP

#include "common.hpp"

// NOTE: Nothing ever copies one of these, so what they want to be is move only
// callables. `std::function` is used instead because we are on C++20, and it
// additionally demands that the callable be copyable, so a lambda capturing
// anything move only is rejected.
//
// TODO: Once we upgrade to C++23, replace both with `std::move_only_function`.
// Its signature must carry a trailing `const` so that its `operator()` is const
// and the paths that hold one by const reference keep compiling:
// using onSendCallback = std::move_only_function<void(bool sent) const>;
// using onMessageQueuedCallback =
//   std::move_only_function<void(bool queued, bool isSendBufferFull) const>;

/**
 * Told whether a packet handed over for sending actually left the socket.
 *
 * It travels by value down the send path and is moved at every step, so it
 * cannot be leaked nor released twice, and any failure path can answer `false`
 * by just invoking it. An empty one means that nobody wants to know, which is
 * the common case, so every send path must tolerate it.
 */
using onSendCallback = std::function<void(bool sent)>;

/**
 * Told whether a message (such as SCTP message) was queued for sending, and
 * whether it was the send buffer being full what kept it from being queued.
 */
using onMessageQueuedCallback = std::function<void(bool queued, bool isSendBufferFull)>;

#endif
