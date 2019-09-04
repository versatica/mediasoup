/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "webrtc::PacketRouter"
// #define MS_LOG_DEV

#include "modules/pacing/packet_router.h"

#include "Logger.hpp"

namespace webrtc {

void PacketRouter::SendPacket(RTC::RtpPacket* packet,
                              const PacedPacketInfo& cluster_info) {
  // jmillan: Remove.
  return;

  // jmillan: TODO. This is the Transport::SendRtpPacket()
  /*
  if (SendRtpPacket(packet))
    return;

  MS_WARN_TAG(bwe, "Failed to send packet, "
                   "ssrc:%" PRIu32 ", sequence number:%" PRIu16,
                      packet->GetSsrc(),
                      packet->GetSequenceNumber());
  */
}

std::vector<RTC::RtpPacket*> PacketRouter::GeneratePadding(
    size_t target_size_bytes) {
  // jmillan: somewhow generate padding packets....

  return {};
}

}  // namespace webrtc
