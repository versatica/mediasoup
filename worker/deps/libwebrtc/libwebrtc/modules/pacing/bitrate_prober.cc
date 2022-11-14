/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "webrtc::BitrateProber"
// #define MS_LOG_DEV_LEVEL 3

#include "modules/pacing/bitrate_prober.h"

#include "Logger.hpp"

#include <absl/memory/memory.h>
#include <algorithm>

// TODO: jeje
#define TODO_PRINT_PROBING_STATE() \
  switch (probing_state_) \
  { \
    case ProbingState::kDisabled: \
      MS_DUMP("--- probing_state_:kDisabled, clusters_.size():%zu", clusters_.size()); \
      break; \
    case ProbingState::kInactive: \
      MS_DUMP("--- probing_state_:kInactive, clusters_.size():%zu", clusters_.size()); \
      break; \
    case ProbingState::kActive: \
      MS_DUMP("--- probing_state_:kActive, clusters_.size():%zu", clusters_.size()); \
      break; \
    case ProbingState::kSuspended: \
      MS_DUMP("--- probing_state_:kSuspended, clusters_.size():%zu", clusters_.size()); \
      break; \
  }
#undef TODO_PRINT_PROBING_STATE
  #define TODO_PRINT_PROBING_STATE() {}


namespace webrtc {

namespace {
constexpr TimeDelta kProbeClusterTimeout = TimeDelta::Seconds<5>();

}  // namespace

BitrateProberConfig::BitrateProberConfig(
    const WebRtcKeyValueConfig* key_value_config)
    : min_probe_delta("min_probe_delta", TimeDelta::Millis<2>()),
      max_probe_delay("max_probe_delay", TimeDelta::Millis<10>()),
      min_packet_size("min_packet_size", DataSize::Bytes<200>()) {
  ParseFieldTrial({&min_probe_delta, &max_probe_delay, &min_packet_size},
                  key_value_config->Lookup("WebRTC-Bwe-ProbingBehavior"));
}

BitrateProber::~BitrateProber() {
  // RTC_HISTOGRAM_COUNTS_1000("WebRTC.BWE.Probing.TotalProbeClustersRequested",
                            // total_probe_count_);
  // RTC_HISTOGRAM_COUNTS_1000("WebRTC.BWE.Probing.TotalFailedProbeClusters",
                            // total_failed_probe_count_);
}

BitrateProber::BitrateProber(const WebRtcKeyValueConfig& field_trials)
    : probing_state_(ProbingState::kDisabled),
      next_probe_time_(Timestamp::PlusInfinity()),
      total_probe_count_(0),
      total_failed_probe_count_(0),
      config_(&field_trials) {
  SetEnabled(true);

  // TODO: jeje
  TODO_PRINT_PROBING_STATE();
}

void BitrateProber::SetEnabled(bool enable) {
  if (enable) {
    if (probing_state_ == ProbingState::kDisabled) {
      probing_state_ = ProbingState::kInactive;
      MS_DEBUG_TAG(bwe, "Bandwidth probing enabled, set to inactive");
    }
  } else {
    probing_state_ = ProbingState::kDisabled;
    MS_DEBUG_TAG(bwe, "Bandwidth probing disabled");
  }

  // TODO: jeje
  TODO_PRINT_PROBING_STATE();
}

void BitrateProber::OnIncomingPacket(DataSize packet_size) {
  // Don't initialize probing unless we have something large enough to start
  // probing.
  // Note that the pacer can send several packets at once when sending a probe,
  // and thus, packets can be smaller than needed for a probe.
  if (probing_state_ == ProbingState::kInactive && !clusters_.empty() &&
      packet_size >=
          std::min(RecommendedMinProbeSize(), config_.min_packet_size.Get())) {
    // Send next probe right away.
    next_probe_time_ = Timestamp::MinusInfinity();
    probing_state_ = ProbingState::kActive;
  }

  // TODO: jeje
  TODO_PRINT_PROBING_STATE();
}

void BitrateProber::CreateProbeCluster(const ProbeClusterConfig& cluster_config) {
  // RTC_DCHECK(probing_state_ != ProbingState::kDisabled);
  // RTC_DCHECK_GT(bitrate_bps, 0);
  MS_ASSERT(probing_state_ != ProbingState::kDisabled, "probing disabled");
  MS_ASSERT(cluster_config.target_data_rate.bps() > 0, "bitrate must be > 0");

  total_probe_count_++;
  while (!clusters_.empty() &&
         cluster_config.at_time - clusters_.front().requested_at >
             kProbeClusterTimeout) {
    clusters_.pop();
    total_failed_probe_count_++;
  }

  ProbeCluster cluster;
  cluster.requested_at = cluster_config.at_time;
  cluster.pace_info.probe_cluster_min_probes =
      cluster_config.target_probe_count;
  cluster.pace_info.probe_cluster_min_bytes =
      (cluster_config.target_data_rate * cluster_config.target_duration)
          .bytes();
  // RTC_DCHECK_GE(cluster.pace_info.probe_cluster_min_bytes, 0);

	MS_ASSERT(cluster.pace_info.probe_cluster_min_bytes >= 0, "cluster min bytes must be >= 0");
  cluster.pace_info.send_bitrate_bps = cluster_config.target_data_rate.bps();
  cluster.pace_info.probe_cluster_id = cluster_config.id;
  clusters_.push(cluster);

  MS_DEBUG_DEV("probe cluster [bitrate:%d, min bytes:%d, min probes:%d]",
               cluster.pace_info.send_bitrate_bps,
               cluster.pace_info.probe_cluster_min_bytes,
               cluster.pace_info.probe_cluster_min_probes);

  // If we are already probing, continue to do so. Otherwise set it to
  // kInactive and wait for OnIncomingPacket to start the probing.
  if (probing_state_ != ProbingState::kActive)
    probing_state_ = ProbingState::kInactive;

  // TODO (ibc): We need to send probation even if there is no real packets, so add
  // this code (taken from `OnIncomingPacket()` above) also here.
/*  if (probing_state_ == ProbingState::kInactive && !clusters_.empty()) {
    // Send next probe right away.
    next_probe_time_ms_ = -1;
    probing_state_ = ProbingState::kActive;
  }*/

  // TODO: jeje
  TODO_PRINT_PROBING_STATE();
}

Timestamp BitrateProber::NextProbeTime(Timestamp now) const {
	// TODO: jeje
	TODO_PRINT_PROBING_STATE();
  // Probing is not active or probing is already complete.
  if (probing_state_ != ProbingState::kActive || clusters_.empty()) {
    return Timestamp::PlusInfinity();
  }

  return next_probe_time_;
}

absl::optional<PacedPacketInfo> BitrateProber::CurrentCluster(Timestamp now) {
  if (clusters_.empty() || probing_state_ != ProbingState::kActive) {
    return absl::nullopt;
  }

  if (next_probe_time_.IsFinite() &&
      now - next_probe_time_ > config_.max_probe_delay.Get()) {
		MS_WARN_TAG(bwe, "probe delay too high [next_ms:%" PRIi64 ", now_ms:%" PRIi64 "], discarding probe cluster.",
			          next_probe_time_.ms(),
			          now.ms());
    clusters_.pop();
    if (clusters_.empty()) {
      probing_state_ = ProbingState::kSuspended;
      return absl::nullopt;
    }
  }

  PacedPacketInfo info = clusters_.front().pace_info;
  info.probe_cluster_bytes_sent = clusters_.front().sent_bytes;
  return info;
}

DataSize BitrateProber::RecommendedMinProbeSize() const {
  if (clusters_.empty()) {
    return DataSize::Zero();
  }
  DataRate send_rate =
      DataRate::bps(clusters_.front().pace_info.send_bitrate_bps);
  return send_rate * config_.min_probe_delta;
}

void BitrateProber::ProbeSent(Timestamp now, DataSize size) {
  // RTC_DCHECK(probing_state_ == ProbingState::kActive);
  // RTC_DCHECK(!size.IsZero());

  if (!clusters_.empty()) {
    ProbeCluster* cluster = &clusters_.front();
    if (cluster->sent_probes == 0) {
      // RTC_DCHECK(cluster->started_at.IsInfinite());

			MS_ASSERT(cluster->started_at.IsInfinite(), "cluster started time must not be -1");
      cluster->started_at = now;
    }
    cluster->sent_bytes += size.bytes<int>();
    cluster->sent_probes += 1;
    next_probe_time_ = CalculateNextProbeTime(*cluster);
    if (cluster->sent_bytes >= cluster->pace_info.probe_cluster_min_bytes &&
        cluster->sent_probes >= cluster->pace_info.probe_cluster_min_probes) {
      // RTC_HISTOGRAM_COUNTS_100000("WebRTC.BWE.Probing.ProbeClusterSizeInBytes",
                                  // cluster->sent_bytes);
      // RTC_HISTOGRAM_COUNTS_100("WebRTC.BWE.Probing.ProbesPerCluster",
                               // cluster->sent_probes);
      // RTC_HISTOGRAM_COUNTS_10000("WebRTC.BWE.Probing.TimePerProbeCluster",
                                 // now_ms - cluster->time_started_ms);

      clusters_.pop();
    }
    if (clusters_.empty())
      probing_state_ = ProbingState::kSuspended;

    // TODO: jeje
    TODO_PRINT_PROBING_STATE();
  }
}

Timestamp BitrateProber::CalculateNextProbeTime(
    const ProbeCluster& cluster) const {
  // RTC_CHECK_GT(cluster.pace_info.send_bitrate_bps, 0);
  // RTC_CHECK(cluster.started_at.IsFinite());
	MS_ASSERT(cluster.pace_info.send_bitrate_bps > 0, "cluster.pace_info.send_bitrate_bps must be > 0");
	MS_ASSERT(cluster.started_at.IsFinite(), "cluster.time_started_ms must be > 0");

  // Compute the time delta from the cluster start to ensure probe bitrate stays
  // close to the target bitrate. Result is in milliseconds.
  DataSize sent_bytes = DataSize::bytes(cluster.sent_bytes);
  DataRate send_bitrate =
      DataRate::bps(cluster.pace_info.send_bitrate_bps);

  TimeDelta delta = sent_bytes / send_bitrate;
  return cluster.started_at + delta;
}

}  // namespace webrtc
