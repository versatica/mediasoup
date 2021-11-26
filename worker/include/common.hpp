#ifndef MS_COMMON_HPP
#define MS_COMMON_HPP

#include <algorithm>  // std::transform(), std::find(), std::min(), std::max()
#include <cinttypes>  // PRIu64, etc
#include <cstddef>    // size_t
#include <cstdint>    // uint8_t, etc
#include <functional> // std::function
#include <memory>     // std::addressof()
#ifdef _WIN32
#include <winsock2.h>
// avoid uv/win.h: error C2628 'intptr_t' followed by 'int' is illegal.
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define SSIZE_MAX INTPTR_MAX
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#endif
#else
#include <arpa/inet.h>  // htonl(), htons(), ntohl(), ntohs()
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#include <sys/socket.h> // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#endif

using ChannelReadCtx    = void*;
using ChannelReadFreeFn = void (*)(uint8_t*, uint32_t, size_t);
// Returns `ChannelReadFree` on successful read that must be used to free `message`.
using ChannelReadFn = ChannelReadFreeFn (*)(
  uint8_t** /* message */,
  uint32_t* /* messageLen */,
  size_t* /* messageCtx */,
  // This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
  // data to read.
  const void* /* handle */,
  ChannelReadCtx /* ctx */);

using ChannelWriteCtx = void*;
using ChannelWriteFn =
  void (*)(const uint8_t* /* message */, uint32_t /* messageLen */, ChannelWriteCtx /* ctx */);

using PayloadChannelReadCtx    = void*;
using PayloadChannelReadFreeFn = void (*)(uint8_t*, uint32_t, size_t);
// Returns `PayloadChannelReadFree` on successful read that must be used to free `message` and `payload`.
using PayloadChannelReadFn = PayloadChannelReadFreeFn (*)(
  uint8_t** /* message */,
  uint32_t* /* messageLen */,
  size_t* /* messageCtx */,
  uint8_t** /* payload */,
  uint32_t* /* payloadLen */,
  size_t* /* payloadCapacity */,
  // This is `uv_async_t` handle that can be called later with `uv_async_send()` when there is more
  // data to read.
  const void* /* handle */,
  PayloadChannelReadCtx /* ctx */);

using PayloadChannelWriteCtx = void*;
using PayloadChannelWriteFn  = void (*)(
  const uint8_t* /* message */,
  uint32_t /* messageLen */,
  const uint8_t* /* payload */,
  uint32_t /* payloadLen */,
  ChannelWriteCtx /* ctx */);

#endif
