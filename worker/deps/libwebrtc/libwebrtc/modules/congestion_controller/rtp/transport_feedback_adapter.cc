/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "webrtc::TransportFeedbackAdapter"
// #define MS_LOG_DEV_LEVEL 3

#include "modules/congestion_controller/rtp/transport_feedback_adapter.h"
#include "api/units/timestamp.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "system_wrappers/source/field_trial.h"
#include "mediasoup_helpers.h"

#include "Logger.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace webrtc {
namespace {

PacketResult NetworkPacketFeedbackFromRtpPacketFeedback(
    const webrtc::PacketFeedback& pf) {
  PacketResult feedback;
  if (pf.arrival_time_ms == webrtc::PacketFeedback::kNotReceived) {
    feedback.receive_time = Timestamp::PlusInfinity();
  } else {
    feedback.receive_time = Timestamp::ms(pf.arrival_time_ms);
  }
  feedback.sent_packet.sequence_number = pf.long_sequence_number;
  feedback.sent_packet.send_time = Timestamp::ms(pf.send_time_ms);
  feedback.sent_packet.size = DataSize::bytes(pf.payload_size);
  feedback.sent_packet.pacing_info = pf.pacing_info;
  feedback.sent_packet.prior_unacked_data =
      DataSize::bytes(pf.unacknowledged_data);
  return feedback;
}
}  // namespace
const int64_t kNoTimestamp = -1;
const int64_t kSendTimeHistoryWindowMs = 60000;

TransportFeedbackAdapter::TransportFeedbackAdapter()
    : allow_duplicates_(field_trial::IsEnabled(
          "WebRTC-TransportFeedbackAdapter-AllowDuplicates")),
      send_time_history_(kSendTimeHistoryWindowMs),
      current_offset_ms_(kNoTimestamp),
      last_timestamp_us_(kNoTimestamp),
      local_net_id_(0),
      remote_net_id_(0) {}

TransportFeedbackAdapter::~TransportFeedbackAdapter() {
}

void TransportFeedbackAdapter::AddPacket(const RtpPacketSendInfo& packet_info,
                                         size_t overhead_bytes,
                                         Timestamp creation_time) {
  {
    PacketFeedback packet_feedback(
        creation_time.ms(), packet_info.transport_sequence_number,
        packet_info.length + overhead_bytes, local_net_id_, remote_net_id_,
        packet_info.pacing_info);
    if (packet_info.has_rtp_sequence_number) {
      packet_feedback.ssrc = packet_info.ssrc;
      packet_feedback.rtp_sequence_number = packet_info.rtp_sequence_number;
    }

    // MS_NOTE: TODO remove.
    // MS_DUMP("packet_feedback.arrival_time_ms: %" PRIi64, packet_feedback.arrival_time_ms);
    // MS_DUMP("packet_feedback.send_time_ms: %" PRIi64, packet_feedback.send_time_ms);
    // MS_DUMP("packet_feedback.sequence_number: %" PRIu16, packet_feedback.sequence_number);
    // MS_DUMP("packet_feedback.long_sequence_number: %" PRIi64, packet_feedback.long_sequence_number);
    // MS_DUMP("packet_feedback.payload_size: %zu", packet_feedback.payload_size);
    // MS_DUMP("packet_feedback.unacknowledged_data: %zu", packet_feedback.unacknowledged_data);
    // MS_DUMP("packet_feedback.local_net_id: %" PRIu16, packet_feedback.local_net_id);
    // MS_DUMP("packet_feedback.remote_net_id: %" PRIu16, packet_feedback.remote_net_id);
    // MS_DUMP("packet_feedback.ssrc: %" PRIu32, packet_feedback.ssrc.value());
    // MS_DUMP("packet_feedback.rtp_sequence_number: %" PRIu16, packet_feedback.rtp_sequence_number);

    send_time_history_.RemoveOld(creation_time.ms());
    send_time_history_.AddNewPacket(std::move(packet_feedback));
  }

  {
    for (auto* observer : observers_) {
      observer->OnPacketAdded(packet_info.ssrc,
                              packet_info.transport_sequence_number);
    }
  }
}
absl::optional<SentPacket> TransportFeedbackAdapter::ProcessSentPacket(
    const rtc::SentPacket& sent_packet) {
  // TODO(srte): Only use one way to indicate that packet feedback is used.
  if (sent_packet.info.included_in_feedback || sent_packet.packet_id != -1) {
    SendTimeHistory::Status send_status = send_time_history_.OnSentPacket(
        sent_packet.packet_id, sent_packet.send_time_ms);
    absl::optional<PacketFeedback> packet;
    if (allow_duplicates_ ||
        send_status != SendTimeHistory::Status::kDuplicate) {
      packet = send_time_history_.GetPacket(sent_packet.packet_id);
    }

    if (packet) {
      SentPacket msg;
      msg.size = DataSize::bytes(packet->payload_size);
      msg.send_time = Timestamp::ms(packet->send_time_ms);
      msg.sequence_number = packet->long_sequence_number;
      msg.prior_unacked_data = DataSize::bytes(packet->unacknowledged_data);
      msg.data_in_flight =
          send_time_history_.GetOutstandingData(local_net_id_, remote_net_id_);
      return msg;
    }
  } else if (sent_packet.info.included_in_allocation) {
    send_time_history_.AddUntracked(sent_packet.info.packet_size_bytes,
                                    sent_packet.send_time_ms);
  }
  return absl::nullopt;
}

absl::optional<TransportPacketsFeedback>
TransportFeedbackAdapter::ProcessTransportFeedback(
    const RTC::RTCP::FeedbackRtpTransportPacket& feedback,
    Timestamp feedback_receive_time) {
  DataSize prior_in_flight = GetOutstandingData();

  last_packet_feedback_vector_ =
      GetPacketFeedbackVector(feedback, feedback_receive_time);
  {
    for (auto* observer : observers_) {
      observer->OnPacketFeedbackVector(last_packet_feedback_vector_);
    }
  }

  std::vector<PacketFeedback> feedback_vector = last_packet_feedback_vector_;
  if (feedback_vector.empty())
    return absl::nullopt;

  TransportPacketsFeedback msg;
  for (const PacketFeedback& rtp_feedback : feedback_vector) {
    if (rtp_feedback.send_time_ms != PacketFeedback::kNoSendTime) {
      auto feedback = NetworkPacketFeedbackFromRtpPacketFeedback(rtp_feedback);
      MS_DEBUG_DEV("feedback received for RTP packet: [seq_num: %" PRIi64 ", send_time:%" PRIi64 ", size: %lld, feedback.receive_time:%" PRIi64,
          feedback.sent_packet.sequence_number,
          feedback.sent_packet.send_time.ms(),
          feedback.sent_packet.size.bytes(),
          feedback.receive_time.ms());

      msg.packet_feedbacks.push_back(feedback);
    } else if (rtp_feedback.arrival_time_ms == PacketFeedback::kNotReceived) {
      MS_DEBUG_DEV("--- rtp_feedback.arrival_time_ms == PacketFeedback::kNotReceived ---");
      msg.sendless_arrival_times.push_back(Timestamp::PlusInfinity());
    } else {
      msg.sendless_arrival_times.push_back(
          Timestamp::ms(rtp_feedback.arrival_time_ms));
    }
  }
  {
    absl::optional<int64_t> first_unacked_send_time_ms =
        send_time_history_.GetFirstUnackedSendTime();
    if (first_unacked_send_time_ms)
      msg.first_unacked_send_time = Timestamp::ms(*first_unacked_send_time_ms);
  }
  msg.feedback_time = feedback_receive_time;
  msg.prior_in_flight = prior_in_flight;
  msg.data_in_flight = GetOutstandingData();

  MS_DEBUG_DEV("prior_in_flight:%lld, data_in_flight:%lld", msg.prior_in_flight.bytes(), msg.data_in_flight.bytes());
  return msg;
}

DataSize TransportFeedbackAdapter::GetOutstandingData() const {
  return send_time_history_.GetOutstandingData(local_net_id_, remote_net_id_);
}

std::vector<PacketFeedback> TransportFeedbackAdapter::GetPacketFeedbackVector(
    const RTC::RTCP::FeedbackRtpTransportPacket& feedback,
    Timestamp feedback_time) {
  // Add timestamp deltas to a local time base selected on first packet arrival.
  // This won't be the true time base, but makes it easier to manually inspect
  // time stamps.
  if (last_timestamp_us_ == kNoTimestamp) {
    current_offset_ms_ = feedback_time.ms();
  } else {
    current_offset_ms_ +=
      mediasoup_helpers::FeedbackRtpTransport::GetBaseDeltaUs(&feedback, last_timestamp_us_) / 1000;
  }
  last_timestamp_us_ =
    mediasoup_helpers::FeedbackRtpTransport::GetBaseTimeUs(&feedback);

  std::vector<PacketFeedback> packet_feedback_vector;
  if (feedback.GetPacketStatusCount() == 0) {
    MS_WARN_DEV("empty transport feedback packet received");
    return packet_feedback_vector;
  }
  packet_feedback_vector.reserve(feedback.GetPacketStatusCount());
  {
    size_t failed_lookups = 0;
    int64_t offset_us = 0;
    int64_t timestamp_ms = 0;
    uint16_t seq_num = feedback.GetBaseSequenceNumber();
    for (const auto& packet : mediasoup_helpers::FeedbackRtpTransport::GetReceivedPackets(&feedback)) {
      // Insert into the vector those unreceived packets which precede this
      // iteration's received packet.
      for (; seq_num != packet.sequence_number(); ++seq_num) {
        PacketFeedback packet_feedback(PacketFeedback::kNotReceived, seq_num);
        // Note: Element not removed from history because it might be reported
        // as received by another feedback.
        if (!send_time_history_.GetFeedback(&packet_feedback, false))
          ++failed_lookups;
        if (packet_feedback.local_net_id == local_net_id_ &&
            packet_feedback.remote_net_id == remote_net_id_) {
          packet_feedback_vector.push_back(packet_feedback);
        }
      }

      // Handle this iteration's received packet.
      offset_us += packet.delta_us();
      timestamp_ms = current_offset_ms_ + (offset_us / 1000);
      PacketFeedback packet_feedback(timestamp_ms, packet.sequence_number());
      if (!send_time_history_.GetFeedback(&packet_feedback, true))
        ++failed_lookups;
      if (packet_feedback.local_net_id == local_net_id_ &&
          packet_feedback.remote_net_id == remote_net_id_) {
        packet_feedback_vector.push_back(packet_feedback);
      }
      ++seq_num;
    }

    if (failed_lookups > 0) {
      MS_WARN_DEV("failed to lookup send time for %zu"
                  " packet%s, send time history too small?",
                  failed_lookups,
                  (failed_lookups > 1 ? "s" : ""));
    }
  }
  return packet_feedback_vector;
}

std::vector<PacketFeedback>
TransportFeedbackAdapter::GetTransportFeedbackVector() const {
  return last_packet_feedback_vector_;
}

}  // namespace webrtc
