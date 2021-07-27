/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_PACING_PACKET_ROUTER_H_
#define MODULES_PACING_PACKET_ROUTER_H_

#include "api/transport/network_types.h"
#include "rtc_base/constructor_magic.h"

#include "RTC/RtpPacket.hpp"

#include <stddef.h>
#include <stdint.h>

namespace webrtc {

// PacketRouter keeps track of rtp send modules to support the pacer.
// In addition, it handles feedback messages, which are sent on a send
// module if possible (sender report), otherwise on receive module
// (receiver report). For the latter case, we also keep track of the
// receive modules.
class PacketRouter {
 public:
  PacketRouter() = default;
  virtual ~PacketRouter() = default;

  virtual void SendPacket(RTC::RtpPacket* packet,
                          const PacedPacketInfo& cluster_info) = 0;

  // MS_NOTE: Changed to return a single RtpPacket pointer (maybe nullptr).
  virtual RTC::RtpPacket* GeneratePadding(size_t target_size_bytes) = 0;
};
}  // namespace webrtc
#endif  // MODULES_PACING_PACKET_ROUTER_H_
