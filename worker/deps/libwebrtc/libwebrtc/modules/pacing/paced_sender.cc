/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/pacing/paced_sender.h"

#include <algorithm>
#include <utility>

#include "absl/memory/memory.h"
// #include "api/task_queue/pending_task_safety_flag.h"
#include "api/transport/network_types.h"
// #include "rtc_base/checks.h"
#include "rtc_base/experiments/field_trial_parser.h"
#include "rtc_base/experiments/field_trial_units.h"
#include "rtc_base/system/unused.h"
// #include "rtc_base/trace_event.h"

namespace webrtc {

namespace {

constexpr const char* kBurstyPacerFieldTrial = "WebRTC-BurstyPacer";

constexpr const char* kSlackedPacedSenderFieldTrial =
    "WebRTC-SlackedPacedSender";

}  // namespace

const int PacedSender::kNoPacketHoldback = -1;

PacedSender::BurstyPacerFlags::BurstyPacerFlags(
    const WebRtcKeyValueConfig& field_trials)
    : burst("burst") {
  ParseFieldTrial({&burst}, field_trials.Lookup(kBurstyPacerFieldTrial));
}

PacedSender::SlackedPacerFlags::SlackedPacerFlags(
    const WebRtcKeyValueConfig& field_trials)
    : allow_low_precision("Enabled"),
      max_low_precision_expected_queue_time("max_queue_time"),
      send_burst_interval("send_burst_interval") {
  ParseFieldTrial({&allow_low_precision, &max_low_precision_expected_queue_time,
                   &send_burst_interval},
                  field_trials.Lookup(kSlackedPacedSenderFieldTrial));
}

PacedSender::PacedSender(
    Clock* clock,
    PacingController::PacketSender* packet_sender,
    const WebRtcKeyValueConfig& field_trials,
    TimeDelta max_hold_back_window,
    int max_hold_back_window_in_packets)
    : clock_(clock),
      bursty_pacer_flags_(field_trials),
      slacked_pacer_flags_(field_trials),
      max_hold_back_window_(slacked_pacer_flags_.allow_low_precision
                                ? PacingController::kMinSleepTime
                                : max_hold_back_window),
      max_hold_back_window_in_packets_(slacked_pacer_flags_.allow_low_precision
                                           ? 0
                                           : max_hold_back_window_in_packets),
      pacing_controller_(packet_sender, field_trials),
      next_process_time_(Timestamp::MinusInfinity()),
      is_started_(false),
      is_shutdown_(false),
      packet_size_(/*alpha=*/0.95),
      include_overhead_(false) {
  // RTC_DCHECK_GE(max_hold_back_window_, PacingController::kMinSleepTime);
  // There are multiple field trials that can affect burst. If multiple bursts
  // are specified we pick the largest of the values.
  absl::optional<TimeDelta> burst = bursty_pacer_flags_.burst.GetOptional();
  if (slacked_pacer_flags_.allow_low_precision &&
      slacked_pacer_flags_.send_burst_interval) {
    TimeDelta slacked_burst = slacked_pacer_flags_.send_burst_interval.Value();
    if (!burst.has_value() || burst.value() < slacked_burst) {
      burst = slacked_burst;
    }
  }
  if (burst.has_value()) {
    pacing_controller_.SetSendBurstInterval(burst.value());
  }
}

PacedSender::~PacedSender() {
	is_shutdown_ = true;
}

void PacedSender::EnsureStarted() {
    is_started_ = true;
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::CreateProbeClusters(
    std::vector<ProbeClusterConfig> probe_cluster_configs) {
        pacing_controller_.CreateProbeClusters(probe_cluster_configs);
        MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::Pause() {
    pacing_controller_.Pause();
}

void PacedSender::Resume() {
    pacing_controller_.Resume();
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetCongested(bool congested) {
    pacing_controller_.SetCongested(congested);
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetPacingRates(DataRate pacing_rate,
                                          DataRate padding_rate) {
	pacing_controller_.SetPacingRates(pacing_rate, padding_rate);
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::EnqueuePackets(
    std::vector<std::unique_ptr<RtpPacketToSend>> packets) {
        for (auto& packet : packets) {
				/* TRACE_EVENT2(TRACE_DISABLED_BY_DEFAULT("webrtc"),
                       "PacedSender::EnqueuePackets::Loop",
                       "sequence_number", packet->SequenceNumber(),
                       "rtp_timestamp", packet->Timestamp());*/

          size_t packet_size = packet->payload_size() + packet->padding_size();
          if (include_overhead_) {
            packet_size += packet->headers_size();
          }
          packet_size_.Apply(1, packet_size);
          RTC_DCHECK_GE(packet->capture_time(), Timestamp::Zero());
          pacing_controller_.EnqueuePacket(std::move(packet));
        }
        MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetAccountForAudioPackets(bool account_for_audio) {
    pacing_controller_.SetAccountForAudioPackets(account_for_audio);
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetIncludeOverhead() {
    include_overhead_ = true;
    pacing_controller_.SetIncludeOverhead();
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetTransportOverhead(DataSize overhead_per_packet) {
    pacing_controller_.SetTransportOverhead(overhead_per_packet);
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

void PacedSender::SetQueueTimeLimit(TimeDelta limit) {
    pacing_controller_.SetQueueTimeLimit(limit);
    MaybeProcessPackets(Timestamp::MinusInfinity());
}

TimeDelta PacedSender::ExpectedQueueTime() const {
  return GetStats().expected_queue_time;
}

DataSize PacedSender::QueueSizeData() const {
  return GetStats().queue_size;
}

absl::optional<Timestamp> PacedSender::FirstSentPacketTime() const {
  return GetStats().first_sent_packet_time;
}

TimeDelta PacedSender::OldestPacketWaitTime() const {
  Timestamp oldest_packet = GetStats().oldest_packet_enqueue_time;
  if (oldest_packet.IsInfinite()) {
    return TimeDelta::Zero();
  }

  // (webrtc:9716): The clock is not always monotonic.
  Timestamp current = clock_->CurrentTime();
  if (current < oldest_packet) {
    return TimeDelta::Zero();
  }

  return current - oldest_packet;
}

void PacedSender::OnStatsUpdated(const Stats& stats) {
  current_stats_ = stats;
}

void PacedSender::MaybeProcessPackets(
    Timestamp scheduled_process_time) {
/*  RTC_DCHECK_RUN_ON(&task_queue_);

  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("webrtc"),
               "PacedSender::MaybeProcessPackets");*/

  if (is_shutdown_ || !is_started_) {
    return;
  }

  Timestamp next_send_time = pacing_controller_.NextSendTime();
  // RTC_DCHECK(next_send_time.IsFinite());
  const Timestamp now = clock_->CurrentTime();
  TimeDelta early_execute_margin =
      pacing_controller_.IsProbing()
          ? PacingController::kMaxEarlyProbeProcessing
          : TimeDelta::Zero();

  // Process packets and update stats.
  while (next_send_time <= now + early_execute_margin) {
    pacing_controller_.ProcessPackets();
    next_send_time = pacing_controller_.NextSendTime();
    // RTC_DCHECK(next_send_time.IsFinite());

    // Probing state could change. Get margin after process packets.
    early_execute_margin = pacing_controller_.IsProbing()
                               ? PacingController::kMaxEarlyProbeProcessing
                               : TimeDelta::Zero();
  }
  UpdateStats();

  // Ignore retired scheduled task, otherwise reset `next_process_time_`.
  if (scheduled_process_time.IsFinite()) {
    if (scheduled_process_time != next_process_time_) {
      return;
    }
    next_process_time_ = Timestamp::MinusInfinity();
  }

  // Do not hold back in probing.
  TimeDelta hold_back_window = TimeDelta::Zero();
  if (!pacing_controller_.IsProbing()) {
    hold_back_window = max_hold_back_window_;
    DataRate pacing_rate = pacing_controller_.pacing_rate();
    if (max_hold_back_window_in_packets_ != kNoPacketHoldback &&
        !pacing_rate.IsZero() &&
        packet_size_.filtered() != rtc::ExpFilter::kValueUndefined) {
      TimeDelta avg_packet_send_time =
          DataSize::Bytes(packet_size_.filtered()) / pacing_rate;
      hold_back_window =
          std::min(hold_back_window,
                   avg_packet_send_time * max_hold_back_window_in_packets_);
    }
  }

  // Calculate next process time.
  TimeDelta time_to_next_process =
      std::max(hold_back_window, next_send_time - now - early_execute_margin);
  next_send_time = now + time_to_next_process;

  // If no in flight task or in flight task is later than `next_send_time`,
  // schedule a new one. Previous in flight task will be retired.
  if (next_process_time_.IsMinusInfinity() ||
      next_process_time_ > next_send_time) {
    // Prefer low precision if allowed and not probing.
    TaskQueueBase::DelayPrecision precision =
        slacked_pacer_flags_.allow_low_precision &&
                !pacing_controller_.IsProbing()
            ? TaskQueueBase::DelayPrecision::kLow
            : TaskQueueBase::DelayPrecision::kHigh;
    // Check for cases where we need high precision.
    if (precision == TaskQueueBase::DelayPrecision::kLow) {
      auto& packets_per_type =
          pacing_controller_.SizeInPacketsPerRtpPacketMediaType();
      bool audio_or_retransmission_packets_in_queue =
          packets_per_type[static_cast<size_t>(RtpPacketMediaType::kAudio)] >
              0 ||
          packets_per_type[static_cast<size_t>(
              RtpPacketMediaType::kRetransmission)] > 0;
      bool queue_time_too_large =
          slacked_pacer_flags_.max_low_precision_expected_queue_time &&
          pacing_controller_.ExpectedQueueTime() >=
              slacked_pacer_flags_.max_low_precision_expected_queue_time
                  .Value();
      if (audio_or_retransmission_packets_in_queue || queue_time_too_large) {
        precision = TaskQueueBase::DelayPrecision::kHigh;
      }
    }

    task_queue_.TaskQueueForDelayedTasks()->PostDelayedTaskWithPrecision(
        precision,
        task_queue_.MaybeSafeTask(
            safety_.flag(),
            [this, next_send_time]() { MaybeProcessPackets(next_send_time); }),
        time_to_next_process.RoundUpTo(TimeDelta::Millis(1)));
    next_process_time_ = next_send_time;
  }
}

int64_t PacedSender::TimeUntilNextProcess() {
	Timestamp next_send_time = pacing_controller_.NextSendTime();
	TimeDelta sleep_time =
		std::max(TimeDelta::Zero(), next_send_time - clock_->CurrentTime());
	if (process_mode_ == PacingController::ProcessMode::kDynamic) {
		return std::max(sleep_time, PacingController::kMinSleepTime).ms();
	}
	return sleep_time.ms();
}
void PacedSender::Process() {
	pacing_controller_.ProcessPackets();
}

void PacedSender::UpdateStats() {
  Stats new_stats;
  new_stats.expected_queue_time = pacing_controller_.ExpectedQueueTime();
  new_stats.first_sent_packet_time = pacing_controller_.FirstSentPacketTime();
  new_stats.oldest_packet_enqueue_time =
      pacing_controller_.OldestPacketEnqueueTime();
  new_stats.queue_size = pacing_controller_.QueueSizeData();
  OnStatsUpdated(new_stats);
}

PacedSender::Stats PacedSender::GetStats() const {
  return current_stats_;
}

}  // namespace webrtc
