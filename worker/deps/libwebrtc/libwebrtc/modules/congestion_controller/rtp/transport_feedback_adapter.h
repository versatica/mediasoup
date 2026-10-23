/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_RTP_TRANSPORT_FEEDBACK_ADAPTER_H_
#define MODULES_CONGESTION_CONTROLLER_RTP_TRANSPORT_FEEDBACK_ADAPTER_H_

#include "api/transport/network_types.h"
#include "modules/congestion_controller/rtp/send_time_history.h"
#include "rtc_base/network/sent_packet.h"

#include "RTC/RTCP/FeedbackRtpTransport.hpp"

#include <deque>
#include <vector>

namespace webrtc {

class PacketFeedbackObserver;
struct RtpPacketSendInfo;

class TransportFeedbackAdapter {
 public:
  TransportFeedbackAdapter();
  virtual ~TransportFeedbackAdapter();

  void AddPacket(const RtpPacketSendInfo& packet_info,
                 size_t overhead_bytes,
                 Timestamp creation_time);
  absl::optional<SentPacket> ProcessSentPacket(
      const rtc::SentPacket& sent_packet);

  absl::optional<TransportPacketsFeedback> ProcessTransportFeedback(
      const RTC::RTCP::FeedbackRtpTransportPacket& feedback,
      Timestamp feedback_time);

  std::vector<PacketFeedback> GetTransportFeedbackVector() const;

  DataSize GetOutstandingData() const;

 private:
  void OnTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket& feedback);

  std::vector<PacketFeedback> GetPacketFeedbackVector(
      const RTC::RTCP::FeedbackRtpTransportPacket& feedback,
      Timestamp feedback_time);

  const bool allow_duplicates_;

  SendTimeHistory send_time_history_;
  int64_t current_offset_ms_;
  int64_t last_timestamp_us_;
  std::vector<PacketFeedback> last_packet_feedback_vector_;
  // MS_NOTE: local_net_id_ and remote_net_id_ are not set.
  uint16_t local_net_id_;
  uint16_t remote_net_id_;

  std::vector<PacketFeedbackObserver*> observers_;
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_RTP_TRANSPORT_FEEDBACK_ADAPTER_H_
