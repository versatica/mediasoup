/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "RTC/SendTransportController/pacing/paced_sender.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "RTC/SendTransportController/pacing/bitrate_prober.h"
#include "RTC/SendTransportController/pacing/interval_budget.h"
#include "RTC/SendTransportController/rtp_packet_send_result.h"

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/RtpPacket.hpp"

#define MS_CLASS "PacedSender"

namespace webrtc {
namespace {
// Time limit in milliseconds between packet bursts.
const int64_t kDefaultMinPacketLimitMs = 5;
const int64_t kCongestedPacketIntervalMs = 500;
const int64_t kPausedProcessIntervalMs = kCongestedPacketIntervalMs;
const int64_t kMaxElapsedTimeMs = 2000;

// Upper cap on process interval, in case process has not been called in a long
// time.
const int64_t kMaxIntervalTimeMs = 30;

}  // namespace
const float PacedSender::kDefaultPaceMultiplier = 2.5f;

PacedSender::PacedSender(PacketRouter* packet_router,
                         const WebRtcKeyValueConfig* field_trials)
    : packet_router_(packet_router),
      fallback_field_trials_(
          !field_trials ? absl::make_unique<FieldTrialBasedConfig>() : nullptr),
      field_trials_(field_trials ? field_trials : fallback_field_trials_.get()),
      min_packet_limit_ms_("", kDefaultMinPacketLimitMs),
      paused_(false),
      media_budget_(0),
      padding_budget_(0),
      prober_(*field_trials_),
      probing_send_failure_(false),
      pacing_bitrate_kbps_(0),
      time_last_process_ms_(DepLibUV::GetTime()),
      last_send_time_ms_(DepLibUV::GetTime()),
      first_sent_packet_ms_(-1),
      packet_counter_(0),
      account_for_audio_(false) {
  ParseFieldTrial({&min_packet_limit_ms_},
                  field_trials_->Lookup("WebRTC-Pacer-MinPacketLimitMs"));
  UpdateBudgetWithElapsedTime(min_packet_limit_ms_);
}

void PacedSender::CreateProbeCluster(int bitrate_bps, int cluster_id) {
  prober_.CreateProbeCluster(bitrate_bps, DepLibUV::GetTime(), cluster_id);
}

void PacedSender::Pause() {
  if (!paused_)
    MS_DEBUG_TAG(bwe, "PacedSender paused");

  paused_ = true;
}

void PacedSender::Resume() {
  if (paused_)
    MS_DEBUG_TAG(bwe, "PacedSender resumed");

  paused_ = false;
}

void PacedSender::SetCongestionWindow(int64_t congestion_window_bytes) {
  congestion_window_bytes_ = congestion_window_bytes;
}

void PacedSender::UpdateOutstandingData(int64_t outstanding_bytes) {
  outstanding_bytes_ = outstanding_bytes;
}

bool PacedSender::Congested() const {
  if (congestion_window_bytes_ == kNoCongestionWindow)
    return false;
  return outstanding_bytes_ >= congestion_window_bytes_;
}

void PacedSender::SetProbingEnabled(bool enabled) {
  // RTC_CHECK_EQ(0, packet_counter_);
  prober_.SetEnabled(enabled);
}

void PacedSender::SetPacingRates(uint32_t pacing_rate_bps,
                                 uint32_t padding_rate_bps) {
  // RTC_DCHECK(pacing_rate_bps > 0);
  pacing_bitrate_kbps_ = pacing_rate_bps / 1000;
  padding_budget_.set_target_rate_kbps(padding_rate_bps / 1000);

  MS_DEBUG_TAG(bwe, "pacer_updated pacing_kbps=%" PRIu32
                    " padding_budget_kbps=%" PRIu32,
                      pacing_bitrate_kbps_,
                      padding_rate_bps / 1000);
}

void PacedSender::InsertPacket(size_t bytes) {
  if (pacing_bitrate_kbps_ == 0)
      MS_WARN_TAG(bwe, "SetPacingRates() must be called before InsertPacket()");

  prober_.OnIncomingPacket(bytes);

  packet_counter_++;
}

void PacedSender::SetAccountForAudioPackets(bool account_for_audio) {
  account_for_audio_ = account_for_audio;
}

int64_t PacedSender::TimeUntilNextProcess() {
  int64_t elapsed_time_ms =
      DepLibUV::GetTime() - time_last_process_ms_;
  // When paused we wake up every 500 ms to send a padding packet to ensure
  // we won't get stuck in the paused state due to no feedback being received.
  if (paused_)
    return std::max<int64_t>(kPausedProcessIntervalMs - elapsed_time_ms, 0);

  if (prober_.IsProbing()) {
    int64_t ret = prober_.TimeUntilNextProbe(DepLibUV::GetTime());
    if (ret > 0 || (ret == 0 && !probing_send_failure_))
      return ret;
  }
  return std::max<int64_t>(min_packet_limit_ms_ - elapsed_time_ms, 0);
}

int64_t PacedSender::UpdateTimeAndGetElapsedMs(int64_t now_ms) {
  int64_t elapsed_time_ms = (now_ms - time_last_process_ms_);
  time_last_process_ms_ = now_ms;
  if (elapsed_time_ms > kMaxElapsedTimeMs) {
    MS_WARN_TAG(bwe, "Elapsed time (%" PRIi64 " ms) longer than expected,"
                     " limiting to %" PRIi64 " ms",
                        elapsed_time_ms,
                        kMaxElapsedTimeMs);
    elapsed_time_ms = kMaxElapsedTimeMs;
  }
  return elapsed_time_ms;
}

// jmillan: TODO: check this method in detail.
void PacedSender::Process() {
  int64_t now_ms = DepLibUV::GetTime();
  int64_t elapsed_time_ms = UpdateTimeAndGetElapsedMs(now_ms);

  if (paused_)
    return;

  if (elapsed_time_ms > 0) {
    int target_bitrate_kbps = pacing_bitrate_kbps_;
    media_budget_.set_target_rate_kbps(target_bitrate_kbps);
    UpdateBudgetWithElapsedTime(elapsed_time_ms);
  }

  if (!prober_.IsProbing())
    return;

  PacedPacketInfo pacing_info;
  absl::optional<size_t> recommended_probe_size;

  pacing_info = prober_.CurrentCluster();
  recommended_probe_size = prober_.RecommendedMinProbeSize();

  size_t bytes_sent = 0;
  std::vector<RTC::RtpPacket*> padding_packets;

  /*
  while (true) {
    // Check if we should send padding.
    size_t padding_bytes_to_add =
      PaddingBytesToAdd(recommended_probe_size, bytes_sent);
    if (padding_bytes_to_add > 0) {
      MS_DUMP("%zu padding bytes to add", padding_bytes_to_add);
      padding_packets =
        packet_router_->GeneratePadding(padding_bytes_to_add);
      if (padding_packets.empty()) {
        // No padding packets were generated, quite send loop.
        break;
      }
    }
    else
    {
      MS_DUMP("no padding bytes to add");

      break;
    }

    auto packet = padding_packets.front();
    RtpPacketSendResult success;

    // jmillan: should we make sure the packet has been sent in order
    // to set 'success'?
    packet_router_->SendPacket(packet, pacing_info);
    success = RtpPacketSendResult::kSuccess;

    if (success == RtpPacketSendResult::kSuccess) {
      // TODO(webrtc:8052): Don't consume media budget on kInvalid.
      bytes_sent += packet->GetSize();
      // Send succeeded, remove it from the queue.
      OnPacketSent(packet);
      if (recommended_probe_size && bytes_sent > *recommended_probe_size)
        break;
    }
  }
  */

  // Check if we should send padding.
  size_t padding_bytes_to_add =
    PaddingBytesToAdd(recommended_probe_size, bytes_sent);
  if (padding_bytes_to_add == 0) {
    MS_DUMP("no padding bytes to add");

    return;
  }
  else
  {
    MS_DUMP("%zu padding bytes to add", padding_bytes_to_add);
    while (bytes_sent < padding_bytes_to_add)
    {
      padding_packets =
        packet_router_->GeneratePadding(padding_bytes_to_add /* not used*/);

      auto packet = padding_packets.front();

      MS_DUMP("sending padding packet for size: %zu", packet->GetSize());
      packet_router_->SendPacket(packet, pacing_info);
      bytes_sent += packet->GetSize();

      // Send succeeded.
      OnPacketSent(packet);
    }
  }

  if (bytes_sent != 0)
  {
    auto now = DepLibUV::GetTime();
    MS_DUMP("OnPaddingSent(bytes_sent:%zu)", bytes_sent);
    OnPaddingSent(now, bytes_sent);
    MS_DUMP("prober_.ProbeSent(now, bytes_sent:%zu)", bytes_sent);
    prober_.ProbeSent(now, bytes_sent);
  }
}

size_t PacedSender::PaddingBytesToAdd(
    absl::optional<size_t> recommended_probe_size,
    size_t bytes_sent) {

  // Don't add padding if congested, even if requested for probing.
  if (Congested()) {
    return 0;
  }

  // We can not send padding unless a normal packet has first been sent. If we
  // do, timestamps get messed up.
  if (packet_counter_ == 0) {
    return 0;
  }

  if (recommended_probe_size) {
    if (*recommended_probe_size > bytes_sent) {
      return *recommended_probe_size - bytes_sent;
    }
    return 0;
  }

  return padding_budget_.bytes_remaining();
}

void PacedSender::OnPacketSent(RTC::RtpPacket* packet) {
  if (first_sent_packet_ms_ == -1)
    first_sent_packet_ms_ = DepLibUV::GetTime();
}

void PacedSender::OnPaddingSent(int64_t now, size_t bytes_sent) {
  if (bytes_sent > 0) {
    UpdateBudgetWithBytesSent(bytes_sent);
  }
  last_send_time_ms_ = now;
}

void PacedSender::UpdateBudgetWithElapsedTime(int64_t delta_time_ms) {
  delta_time_ms = std::min(kMaxIntervalTimeMs, delta_time_ms);
  media_budget_.IncreaseBudget(delta_time_ms);
  padding_budget_.IncreaseBudget(delta_time_ms);
}

void PacedSender::UpdateBudgetWithBytesSent(size_t bytes_sent) {
  outstanding_bytes_ += bytes_sent;
  media_budget_.UseBudget(bytes_sent);
  padding_budget_.UseBudget(bytes_sent);
}

}  // namespace webrtc
