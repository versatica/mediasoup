/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_PACING_PACED_SENDER_H_
#define MODULES_PACING_PACED_SENDER_H_

#include "api/transport/field_trial_based_config.h"
#include "api/transport/network_types.h"
#include "api/transport/webrtc_key_value_config.h"
#include "modules/pacing/bitrate_prober.h"
#include "modules/pacing/interval_budget.h"
#include "modules/pacing/packet_router.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "rtc_base/experiments/field_trial_parser.h"

#include "RTC/RtpPacket.hpp"

#include <absl/types/optional.h>
#include <stddef.h>
#include <stdint.h>
#include <atomic>
#include <memory>

namespace webrtc {

class PacedSender {
 public:
  static constexpr int64_t kNoCongestionWindow = -1;

  // Pacing-rate relative to our target send rate.
  // Multiplicative factor that is applied to the target bitrate to calculate
  // the number of bytes that can be transmitted per interval.
  // Increasing this factor will result in lower delays in cases of bitrate
  // overshoots from the encoder.
  static const float kDefaultPaceMultiplier;

  PacedSender(PacketRouter* packet_router,
              const WebRtcKeyValueConfig* field_trials = nullptr);

  virtual ~PacedSender() = default;

  virtual void CreateProbeCluster(int bitrate_bps, int cluster_id);

  // Temporarily pause all sending.
  void Pause();

  // Resume sending packets.
  void Resume();

  void SetCongestionWindow(int64_t congestion_window_bytes);
  void UpdateOutstandingData(int64_t outstanding_bytes);

  // Enable bitrate probing. Enabled by default, mostly here to simplify
  // testing. Must be called before any packets are being sent to have an
  // effect.
  void SetProbingEnabled(bool enabled);

  // Sets the pacing rates. Must be called once before packets can be sent.
  void SetPacingRates(uint32_t pacing_rate_bps, uint32_t padding_rate_bps);

  // Adds the packet information to the queue and calls TimeToSendPacket
  // when it's time to send.
  // MS_NOTE: defined in "modules/rtp_rtcp/include/rtp_packet_sender.h"
  void InsertPacket(size_t bytes);

  // Currently audio traffic is not accounted by pacer and passed through.
  // With the introduction of audio BWE audio traffic will be accounted for
  // the pacer budget calculation. The audio traffic still will be injected
  // at high priority.
  void SetAccountForAudioPackets(bool account_for_audio);

  // Returns the number of milliseconds until the module want a worker thread
  // to call Process.
  int64_t TimeUntilNextProcess();

  // Process any pending packets in the queue(s).
  void Process();

  void OnPacketSent(size_t size);
  PacedPacketInfo GetPacingInfo();

 private:
  int64_t UpdateTimeAndGetElapsedMs(int64_t now_us);

  // Updates the number of bytes that can be sent for the next time interval.
  void UpdateBudgetWithElapsedTime(int64_t delta_time_in_ms);
  void UpdateBudgetWithBytesSent(size_t bytes);

  size_t PaddingBytesToAdd(absl::optional<size_t> recommended_probe_size,
                           size_t bytes_sent);

  void OnPaddingSent(int64_t now_us, size_t bytes_sent);

  bool Congested() const;

  PacketRouter* const packet_router_;
  const std::unique_ptr<FieldTrialBasedConfig> fallback_field_trials_;
  const WebRtcKeyValueConfig* field_trials_;

  FieldTrialParameter<int> min_packet_limit_ms_;

  bool paused_;
  // This is the media budget, keeping track of how many bits of media
  // we can pace out during the current interval.
  IntervalBudget media_budget_;
  // This is the padding budget, keeping track of how many bits of padding we're
  // allowed to send out during the current interval. This budget will be
  // utilized when there's no media to send.
  IntervalBudget padding_budget_;

  BitrateProber prober_;
  bool probing_send_failure_;

  uint32_t pacing_bitrate_kbps_;

  int64_t time_last_process_us_;
  int64_t first_sent_packet_ms_;

  uint64_t packet_counter_;

  int64_t congestion_window_bytes_ = kNoCongestionWindow;
  int64_t outstanding_bytes_ = 0;

  bool account_for_audio_;
};
}  // namespace webrtc
#endif  // MODULES_PACING_PACED_SENDER_H_
