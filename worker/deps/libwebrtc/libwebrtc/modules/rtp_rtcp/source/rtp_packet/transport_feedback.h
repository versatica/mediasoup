/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TRANSPORT_FEEDBACK_H_
#define MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TRANSPORT_FEEDBACK_H_

#include <cstdint>

namespace webrtc {
namespace rtcp {

// Convert to multiples of 0.25ms.
static constexpr int kDeltaScaleFactor = 250;

class ReceivedPacket {
 public:
  ReceivedPacket(uint16_t sequence_number, int16_t delta_ticks)
      : sequence_number_(sequence_number),
        delta_ticks_(delta_ticks),
        received_(true) {}
  explicit ReceivedPacket(uint16_t sequence_number)
      : sequence_number_(sequence_number), received_(false) {}
  ReceivedPacket(const ReceivedPacket&) = default;
  ReceivedPacket& operator=(const ReceivedPacket&) = default;

  uint16_t sequence_number() const { return sequence_number_; }
  int16_t delta_ticks() const { return delta_ticks_; }
  int32_t delta_us() const { return delta_ticks_ * kDeltaScaleFactor; }
  bool received() const { return received_; }

 private:
  uint16_t sequence_number_;
  int16_t delta_ticks_;
  bool received_;
};
}  // namespace rtcp
}  // namespace webrtc
#endif  // MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_TRANSPORT_FEEDBACK_H_
