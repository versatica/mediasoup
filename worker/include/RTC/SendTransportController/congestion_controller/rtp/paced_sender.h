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

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <memory>

#include "absl/types/optional.h"
#include "api/function_view.h"
#include "api/transport/field_trial_based_config.h"
#include "api/transport/network_types.h"
#include "api/transport/webrtc_key_value_config.h"
#include "modules/include/module.h"
#include "modules/pacing/bitrate_prober.h"
#include "modules/pacing/interval_budget.h"
#include "modules/pacing/packet_router.h"
#include "modules/pacing/round_robin_packet_queue.h"
#include "modules/rtp_rtcp/include/rtp_packet_sender.h"
#include "modules/rtp_rtcp/source/rtp_packet_to_send.h"
#include "modules/utility/include/process_thread.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/experiments/field_trial_parser.h"
#include "rtc_base/thread_annotations.h"

namespace webrtc {
class Clock;
class RtcEventLog;

class PacedSender : public Module, public RtpPacketSender {
 public:
  static constexpr int64_t kNoCongestionWindow = -1;

  // Expected max pacer delay in ms. If ExpectedQueueTimeMs() is higher than
  // this value, the packet producers should wait (eg drop frames rather than
  // encoding them). Bitrate sent may temporarily exceed target set by
  // UpdateBitrate() so that this limit will be upheld.
  static const int64_t kMaxQueueLengthMs;
  // Pacing-rate relative to our target send rate.
  // Multiplicative factor that is applied to the target bitrate to calculate
  // the number of bytes that can be transmitted per interval.
  // Increasing this factor will result in lower delays in cases of bitrate
  // overshoots from the encoder.
  static const float kDefaultPaceMultiplier;

  PacedSender(Clock* clock,
              PacketRouter* packet_router,
              RtcEventLog* event_log,
              const WebRtcKeyValueConfig* field_trials = nullptr);

  ~PacedSender() override;

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
  void InsertPacket(RtpPacketSender::Priority priority,
                    uint32_t ssrc,
                    uint16_t sequence_number,
                    int64_t capture_time_ms,
                    size_t bytes,
                    bool retransmission) override;

  // Adds the packet to the queue and calls PacketRouter::SendPacket() when
  // it's time to send.
  void EnqueuePacket(std::unique_ptr<RtpPacketToSend> packet) override;

  // Currently audio traffic is not accounted by pacer and passed through.
  // With the introduction of audio BWE audio traffic will be accounted for
  // the pacer budget calculation. The audio traffic still will be injected
  // at high priority.
  void SetAccountForAudioPackets(bool account_for_audio);

  // Returns the time since the oldest queued packet was enqueued.
  virtual int64_t QueueInMs() const;

  virtual size_t QueueSizePackets() const;
  virtual int64_t QueueSizeBytes() const;

  // Returns the time when the first packet was sent, or -1 if no packet is
  // sent.
  virtual int64_t FirstSentPacketTimeMs() const;

  // Returns the number of milliseconds it will take to send the current
  // packets in the queue, given the current size and bitrate, ignoring prio.
  virtual int64_t ExpectedQueueTimeMs() const;

  // Returns the number of milliseconds until the module want a worker thread
  // to call Process.
  int64_t TimeUntilNextProcess() override;

  // Process any pending packets in the queue(s).
  void Process() override;

  // Called when the prober is associated with a process thread.
  void ProcessThreadAttached(ProcessThread* process_thread) override;
  void SetQueueTimeLimit(int limit_ms);

 private:
  int64_t UpdateTimeAndGetElapsedMs(int64_t now_us)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  bool ShouldSendKeepalive(int64_t at_time_us) const
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  // Updates the number of bytes that can be sent for the next time interval.
  void UpdateBudgetWithElapsedTime(int64_t delta_time_in_ms)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  void UpdateBudgetWithBytesSent(size_t bytes)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  size_t PaddingBytesToAdd(absl::optional<size_t> recommended_probe_size,
                           size_t bytes_sent)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  RoundRobinPacketQueue::QueuedPacket* GetPendingPacket(
      const PacedPacketInfo& pacing_info)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  void OnPacketSent(RoundRobinPacketQueue::QueuedPacket* packet)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  void OnPaddingSent(size_t padding_sent)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  bool Congested() const RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);
  int64_t TimeMilliseconds() const RTC_EXCLUSIVE_LOCKS_REQUIRED(critsect_);

  Clock* const clock_;
  PacketRouter* const packet_router_;
  const std::unique_ptr<FieldTrialBasedConfig> fallback_field_trials_;
  const WebRtcKeyValueConfig* field_trials_;

  const bool drain_large_queues_;
  const bool send_padding_if_silent_;
  const bool pace_audio_;
  FieldTrialParameter<int> min_packet_limit_ms_;

  rtc::CriticalSection critsect_;
  // TODO(webrtc:9716): Remove this when we are certain clocks are monotonic.
  // The last millisecond timestamp returned by |clock_|.
  mutable int64_t last_timestamp_ms_ RTC_GUARDED_BY(critsect_);
  bool paused_ RTC_GUARDED_BY(critsect_);
  // This is the media budget, keeping track of how many bits of media
  // we can pace out during the current interval.
  IntervalBudget media_budget_ RTC_GUARDED_BY(critsect_);
  // This is the padding budget, keeping track of how many bits of padding we're
  // allowed to send out during the current interval. This budget will be
  // utilized when there's no media to send.
  IntervalBudget padding_budget_ RTC_GUARDED_BY(critsect_);

  BitrateProber prober_ RTC_GUARDED_BY(critsect_);
  bool probing_send_failure_ RTC_GUARDED_BY(critsect_);

  uint32_t pacing_bitrate_kbps_ RTC_GUARDED_BY(critsect_);

  int64_t time_last_process_us_ RTC_GUARDED_BY(critsect_);
  int64_t last_send_time_us_ RTC_GUARDED_BY(critsect_);
  int64_t first_sent_packet_ms_ RTC_GUARDED_BY(critsect_);

  RoundRobinPacketQueue packets_ RTC_GUARDED_BY(critsect_);
  uint64_t packet_counter_ RTC_GUARDED_BY(critsect_);

  int64_t congestion_window_bytes_ RTC_GUARDED_BY(critsect_) =
      kNoCongestionWindow;
  int64_t outstanding_bytes_ RTC_GUARDED_BY(critsect_) = 0;

  // Lock to avoid race when attaching process thread. This can happen due to
  // the Call class setting network state on RtpTransportControllerSend, which
  // in turn calls Pause/Resume on Pacedsender, before actually starting the
  // pacer process thread. If RtpTransportControllerSend is running on a task
  // queue separate from the thread used by Call, this causes a race.
  rtc::CriticalSection process_thread_lock_;
  ProcessThread* process_thread_ RTC_GUARDED_BY(process_thread_lock_) = nullptr;

  int64_t queue_time_limit RTC_GUARDED_BY(critsect_);
  bool account_for_audio_ RTC_GUARDED_BY(critsect_);

  // If true, PacedSender should only reference packets as in legacy mode.
  // If false, PacedSender may have direct ownership of RtpPacketToSend objects.
  // Defaults to true, will be changed to default false soon.
  const bool legacy_packet_referencing_;
};
}  // namespace webrtc
#endif  // MODULES_PACING_PACED_SENDER_H_
