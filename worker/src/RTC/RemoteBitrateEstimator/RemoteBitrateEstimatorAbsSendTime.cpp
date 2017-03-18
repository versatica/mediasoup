/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RemoteBitrateEstimatorAbsSendTime"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp" // Byte::Get3Bytes
#include "Logger.hpp"
#include <math.h>
#include <algorithm>

namespace RTC {

enum {
  kTimestampGroupLengthMs = 5,
  kAbsSendTimeFraction = 18,
  kAbsSendTimeInterArrivalUpshift = 8,
  kInterArrivalShift = kAbsSendTimeFraction + kAbsSendTimeInterArrivalUpshift,
  kInitialProbingIntervalMs = 2000,
  kMinClusterSize = 4,
  kMaxProbePackets = 15,
  kExpectedNumberOfProbes = 3
};

static const double kTimestampToMs = 1000.0 /
    static_cast<double>(1 << kInterArrivalShift);

template<typename K, typename V>
std::vector<K> Keys(const std::map<K, V>& map) {
  std::vector<K> keys;
  keys.reserve(map.size());
  for (typename std::map<K, V>::const_iterator it = map.begin();
      it != map.end(); ++it) {
    keys.push_back(it->first);
  }
  return keys;
}

uint32_t ConvertMsTo24Bits(int64_t time_ms) {
  uint32_t time_24_bits =
      static_cast<uint32_t>(
          ((static_cast<uint64_t>(time_ms) << kAbsSendTimeFraction) + 500) /
          1000) &
      0x00FFFFFF;
  return time_24_bits;
}

bool RemoteBitrateEstimatorAbsSendTime::IsWithinClusterBounds(
    int send_delta_ms,
    const Cluster& cluster_aggregate) {
    if (cluster_aggregate.count == 0)
      return true;
    float cluster_mean = cluster_aggregate.send_mean_ms /
                         static_cast<float>(cluster_aggregate.count);
    return fabs(static_cast<float>(send_delta_ms) - cluster_mean) < 2.5f;
  }

  void RemoteBitrateEstimatorAbsSendTime::AddCluster(
      std::list<Cluster>* clusters,
      Cluster* cluster) {
    cluster->send_mean_ms /= static_cast<float>(cluster->count);
    cluster->recv_mean_ms /= static_cast<float>(cluster->count);
    cluster->mean_size /= cluster->count;
    clusters->push_back(*cluster);
  }

  RemoteBitrateEstimatorAbsSendTime::RemoteBitrateEstimatorAbsSendTime(
      Listener* observer)
      : observer(observer),
        interArrival(),
        estimator(),
        detector(),
        incomingBitrate(),
        incomingBitrateInitialized(false),
        totalProbesReceived(0),
        firstPacketTimeMs(-1),
        lastUpdateMs(-1),
        umaRecorded(false) {
    //MS_DASSERT(this->observer);
    MS_TRACE();
}

void RemoteBitrateEstimatorAbsSendTime::ComputeClusters(
    std::list<Cluster>* clusters) const {
  Cluster current;
  int64_t prev_send_time = -1;
  int64_t prev_recv_time = -1;
  for (std::list<Probe>::const_iterator it = this->probes.begin();
       it != this->probes.end();
       ++it) {
    if (prev_send_time >= 0) {
      int send_delta_ms = it->send_time_ms - prev_send_time;
      int recv_delta_ms = it->recv_time_ms - prev_recv_time;
      if (send_delta_ms >= 1 && recv_delta_ms >= 1) {
        ++current.num_above_min_delta;
      }
      if (!IsWithinClusterBounds(send_delta_ms, current)) {
        if (current.count >= kMinClusterSize)
          AddCluster(clusters, &current);
        current = Cluster();
      }
      current.send_mean_ms += send_delta_ms;
      current.recv_mean_ms += recv_delta_ms;
      current.mean_size += it->payload_size;
      ++current.count;
    }
    prev_send_time = it->send_time_ms;
    prev_recv_time = it->recv_time_ms;
  }
  if (current.count >= kMinClusterSize)
    AddCluster(clusters, &current);
}

std::list<Cluster>::const_iterator
RemoteBitrateEstimatorAbsSendTime::FindBestProbe(
    const std::list<Cluster>& clusters) const {
  int highest_probe_bitrate_bps = 0;
  std::list<Cluster>::const_iterator best_it = clusters.end();
  for (std::list<Cluster>::const_iterator it = clusters.begin();
       it != clusters.end();
       ++it) {
    if (it->send_mean_ms == 0 || it->recv_mean_ms == 0)
      continue;
    if (it->num_above_min_delta > it->count / 2 &&
        (it->recv_mean_ms - it->send_mean_ms <= 2.0f &&
         it->send_mean_ms - it->recv_mean_ms <= 5.0f)) {
      int probe_bitrate_bps =
          std::min(it->GetSendBitrateBps(), it->GetRecvBitrateBps());
      if (probe_bitrate_bps > highest_probe_bitrate_bps) {
        highest_probe_bitrate_bps = probe_bitrate_bps;
        best_it = it;
      }
    } else {
      int send_bitrate_bps = it->mean_size * 8 * 1000 / it->send_mean_ms;
      int recv_bitrate_bps = it->mean_size * 8 * 1000 / it->recv_mean_ms;
      MS_DEBUG_TAG(rbe, "Probe failed, sent at %d"
                        " bps, received at %d"
                        " bps. Mean send delta: %f"
                        " ms, mean recv delta: %f"
                        " ms, num probes: %d",
                        send_bitrate_bps,
                        recv_bitrate_bps,
                        it->send_mean_ms,
                        it->recv_mean_ms,
                        it->count);
      break;
    }
  }
  return best_it;
}

RemoteBitrateEstimatorAbsSendTime::ProbeResult
RemoteBitrateEstimatorAbsSendTime::ProcessClusters(int64_t now_ms) {
  std::list<Cluster> clusters;
  ComputeClusters(&clusters);
  if (clusters.empty()) {
    // If we reach the max number of probe packets and still have no clusters,
    // we will remove the oldest one.
    if (this->probes.size() >= kMaxProbePackets)
      this->probes.pop_front();
    return ProbeResult::kNoUpdate;
  }

  std::list<Cluster>::const_iterator best_it = FindBestProbe(clusters);
  if (best_it != clusters.end()) {
    int probe_bitrate_bps =
        std::min(best_it->GetSendBitrateBps(), best_it->GetRecvBitrateBps());
    // Make sure that a probe sent on a lower bitrate than our estimate can't
    // reduce the estimate.
    if (IsBitrateImproving(probe_bitrate_bps)) {
      MS_DEBUG_TAG(rbe, "Probe successful, sent at %d"
                        " bps, received at %d"
                        " bps. Mean send delta: %f"
                        " ms, mean recv delta: %f"
                        " ms, num probes: %d",
                        best_it->GetSendBitrateBps(),
                        best_it->GetRecvBitrateBps(),
                        best_it->send_mean_ms,
                        best_it->recv_mean_ms,
                        best_it->count);

      this->remoteRate.SetEstimate(probe_bitrate_bps, now_ms);
      return ProbeResult::kBitrateUpdated;
    }
  }

  // Not probing and received non-probe packet, or finished with current set
  // of probes.
  if (clusters.size() >= kExpectedNumberOfProbes)
    this->probes.clear();
  return ProbeResult::kNoUpdate;
}

bool RemoteBitrateEstimatorAbsSendTime::IsBitrateImproving(
    int new_bitrate_bps) const {
  bool initial_probe = !this->remoteRate.ValidEstimate() && new_bitrate_bps > 0;
  bool bitrate_above_estimate =
      this->remoteRate.ValidEstimate() &&
      new_bitrate_bps > static_cast<int>(this->remoteRate.LatestEstimate());
  return initial_probe || bitrate_above_estimate;
}

void RemoteBitrateEstimatorAbsSendTime::IncomingPacket(
    int64_t arrival_time_ms,
    size_t payload_size,
    const RtpPacket& packet,
    const uint8_t* absoluteSendTime) {
  if (!absoluteSendTime) {
    MS_WARN_TAG(rbe, "Incoming packet is missing absolute send time extension!");
    return;
  }
  IncomingPacketInfo(arrival_time_ms, Utils::Byte::Get3Bytes(absoluteSendTime, 0),
                     payload_size, packet.GetSsrc());
}

void RemoteBitrateEstimatorAbsSendTime::IncomingPacketInfo(
    int64_t arrival_time_ms,
    uint32_t send_time_24bits,
    size_t payload_size,
    uint32_t ssrc) {
  MS_ASSERT(send_time_24bits < (1ul << 24), "invalid 'send_time_24bits' value");
  if (!this->umaRecorded) {
    this->umaRecorded = true;
  }
  // Shift up send time to use the full 32 bits that inter_arrival works with,
  // so wrapping works properly.
  uint32_t timestamp = send_time_24bits << kAbsSendTimeInterArrivalUpshift;
  int64_t send_time_ms = static_cast<int64_t>(timestamp) * kTimestampToMs;

  int64_t now_ms = DepLibUV::GetTime();
  // TODO(holmer): SSRCs are only needed for REMB, should be broken out from
  // here.

  // Check if incoming bitrate estimate is valid, and if it needs to be reset.
  uint32_t incoming_bitrate =
      this->incomingBitrate.GetRate(arrival_time_ms);
  if (incoming_bitrate) {
    this->incomingBitrateInitialized = true;
  } else if (this->incomingBitrateInitialized) {
    // Incoming bitrate had a previous valid value, but now not enough data
    // point are left within the current window. Reset incoming bitrate
    // estimator so that the window size will only contain new data points.
    this->incomingBitrate.Reset();
    this->incomingBitrateInitialized = false;
  }
  this->incomingBitrate.Update(payload_size, arrival_time_ms);

  if (this->firstPacketTimeMs == -1)
    this->firstPacketTimeMs = now_ms;

  uint32_t ts_delta = 0;
  int64_t t_delta = 0;
  int size_delta = 0;
  bool update_estimate = false;
  uint32_t target_bitrate_bps = 0;
  std::vector<uint32_t> ssrcs;
  {
    TimeoutStreams(now_ms);
    //MS_DASSERT(this->interArrival.get());
    //MS_DASSERT(this->estimator.get());
    this->ssrcs[ssrc] = now_ms;

    // For now only try to detect probes while we don't have a valid estimate.
    // We currently assume that only packets larger than 200 bytes are paced by
    // the sender.
    const size_t kMinProbePacketSize = 200;
    if (payload_size > kMinProbePacketSize &&
        (!this->remoteRate.ValidEstimate() ||
         now_ms - this->firstPacketTimeMs < kInitialProbingIntervalMs)) {
      // TODO(holmer): Use a map instead to get correct order?
      if (this->totalProbesReceived < kMaxProbePackets) {
        int send_delta_ms = -1;
        int recv_delta_ms = -1;
        if (!this->probes.empty()) {
          send_delta_ms = send_time_ms - this->probes.back().send_time_ms;
          recv_delta_ms = arrival_time_ms - this->probes.back().recv_time_ms;
        }
        MS_DEBUG_TAG(rbe, "Probe packet received: send time=%" PRId64
                          " ms, recv time=%" PRId64
                          " ms, send delta=%d"
                          " ms, recv delta=%d"
                          " ms",
                          send_time_ms,
                          arrival_time_ms,
                          send_delta_ms,
                          recv_delta_ms);

      }
      this->probes.push_back(Probe(send_time_ms, arrival_time_ms, payload_size));
      ++this->totalProbesReceived;
      // Make sure that a probe which updated the bitrate immediately has an
      // effect by calling the onReceiveBitrateChanged callback.
      if (ProcessClusters(now_ms) == ProbeResult::kBitrateUpdated)
        update_estimate = true;
    }
    if (this->interArrival->ComputeDeltas(timestamp, arrival_time_ms, now_ms,
                                      payload_size, &ts_delta, &t_delta,
                                      &size_delta)) {
      double ts_delta_ms = (1000.0 * ts_delta) / (1 << kInterArrivalShift);
      this->estimator->Update(t_delta, ts_delta_ms, size_delta, this->detector.State(),
                         arrival_time_ms);
      this->detector.Detect(this->estimator->GetOffset(), ts_delta_ms,
                       this->estimator->num_of_deltas(), arrival_time_ms);
    }

    if (!update_estimate) {
      // Check if it's time for a periodic update or if we should update because
      // of an over-use.
      if (this->lastUpdateMs == -1 ||
          now_ms - this->lastUpdateMs > this->remoteRate.GetFeedbackInterval()) {
        update_estimate = true;
      } else if (this->detector.State() == kBwOverusing) {
        uint32_t incoming_rate =
            this->incomingBitrate.GetRate(arrival_time_ms);
        if (incoming_rate &&
            this->remoteRate.TimeToReduceFurther(now_ms, incoming_rate)) {
          update_estimate = true;
        }
      }
    }

    if (update_estimate) {
      // The first overuse should immediately trigger a new estimate.
      // We also have to update the estimate immediately if we are overusing
      // and the target bitrate is too high compared to what we are receiving.
      const RateControlInput input(this->detector.State(),
                                   this->incomingBitrate.GetRate(arrival_time_ms),
                                   this->estimator->var_noise());
      this->remoteRate.Update(&input, now_ms);
      target_bitrate_bps = this->remoteRate.UpdateBandwidthEstimate(now_ms);
      update_estimate = this->remoteRate.ValidEstimate();
      ssrcs = Keys(this->ssrcs);
    }
  }
  if (update_estimate) {
    this->lastUpdateMs = now_ms;
    this->observer->onReceiveBitrateChanged(ssrcs, target_bitrate_bps);
  }
}

void RemoteBitrateEstimatorAbsSendTime::Process() {}

int64_t RemoteBitrateEstimatorAbsSendTime::TimeUntilNextProcess() {
  const int64_t kDisabledModuleTime = 1000;
  return kDisabledModuleTime;
}

void RemoteBitrateEstimatorAbsSendTime::TimeoutStreams(int64_t now_ms) {
  for (Ssrcs::iterator it = this->ssrcs.begin(); it != this->ssrcs.end();) {
    if ((now_ms - it->second) > kStreamTimeOutMs) {
      this->ssrcs.erase(it++);
    } else {
      ++it;
    }
  }
  if (this->ssrcs.empty()) {
    // We can't update the estimate if we don't have any active streams.
    this->interArrival.reset(
        new InterArrival((kTimestampGroupLengthMs << kInterArrivalShift) / 1000,
                         kTimestampToMs, true));
    this->estimator.reset(new OveruseEstimator(OverUseDetectorOptions()));
    // We deliberately don't reset the this->firstPacketTimeMs here for now since
    // we only probe for bandwidth in the beginning of a call right now.
  }
}

void RemoteBitrateEstimatorAbsSendTime::OnRttUpdate(int64_t avg_rtt_ms,
                                                    int64_t max_rtt_ms) {
  (void)max_rtt_ms;
  this->remoteRate.SetRtt(avg_rtt_ms);
}

void RemoteBitrateEstimatorAbsSendTime::RemoveStream(uint32_t ssrc) {
  this->ssrcs.erase(ssrc);
}

bool RemoteBitrateEstimatorAbsSendTime::LatestEstimate(
    std::vector<uint32_t>* ssrcs,
    uint32_t* bitrate_bps) const {
  //MS_DASSERT(ssrcs);
  //MS_DASSERT(bitrate_bps);
  if (!this->remoteRate.ValidEstimate()) {
    return false;
  }
  *ssrcs = Keys(this->ssrcs);
  if (this->ssrcs.empty()) {
    *bitrate_bps = 0;
  } else {
    *bitrate_bps = this->remoteRate.LatestEstimate();
  }
  return true;
}

void RemoteBitrateEstimatorAbsSendTime::SetMinBitrate(int min_bitrate_bps) {
  this->remoteRate.SetMinBitrate(min_bitrate_bps);
}
}  // namespace RTC
