/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include "RTC/RemoteBitrateEstimator/InterArrival.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseDetector.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseEstimator.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "Logger.hpp"
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <cassert>


namespace RTC {

struct Probe {
  Probe(int64_t send_time_ms, int64_t recv_time_ms, size_t payload_size)
      : send_time_ms(send_time_ms),
        recv_time_ms(recv_time_ms),
        payload_size(payload_size) {}
  int64_t send_time_ms;
  int64_t recv_time_ms;
  size_t payload_size;
};

struct Cluster {
  Cluster()
      : send_mean_ms(0.0f),
        recv_mean_ms(0.0f),
        mean_size(0),
        count(0),
        num_above_min_delta(0) {}

  int GetSendBitrateBps() const {
    assert(send_mean_ms > 0.0f);
    return mean_size * 8 * 1000 / send_mean_ms;
  }

  int GetRecvBitrateBps() const {
    assert(recv_mean_ms > 0.0f);
    return mean_size * 8 * 1000 / recv_mean_ms;
  }

  float send_mean_ms;
  float recv_mean_ms;
  // TODO(holmer): Add some variance metric as well?
  size_t mean_size;
  int count;
  int num_above_min_delta;
};

class RemoteBitrateEstimatorAbsSendTime : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorAbsSendTime(Listener* observer);
  virtual ~RemoteBitrateEstimatorAbsSendTime() {}

  void IncomingPacket(int64_t arrival_time_ms,
                      size_t payload_size,
                      const RtpPacket& packet,
                      const uint8_t* absoluteSendTime) override;
  // This class relies on Process() being called periodically (at least once
  // every other second) for streams to be timed out properly.
  void Process() override;
  int64_t TimeUntilNextProcess() override;
  void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) override;
  void RemoveStream(uint32_t ssrc) override;
  bool LatestEstimate(std::vector<uint32_t>* ssrcs,
                      uint32_t* bitrate_bps) const override;
  void SetMinBitrate(int min_bitrate_bps) override;

 private:
  typedef std::map<uint32_t, int64_t> Ssrcs;
  enum class ProbeResult { kBitrateUpdated, kNoUpdate };

  static bool IsWithinClusterBounds(int send_delta_ms,
                                    const Cluster& cluster_aggregate);

  static void AddCluster(std::list<Cluster>* clusters, Cluster* cluster);

  void IncomingPacketInfo(int64_t arrival_time_ms,
                          uint32_t send_time_24bits,
                          size_t payload_size,
                          uint32_t ssrc);

  void ComputeClusters(std::list<Cluster>* clusters) const;

  std::list<Cluster>::const_iterator FindBestProbe(
      const std::list<Cluster>& clusters) const;

  // Returns true if a probe which changed the estimate was detected.
  ProbeResult ProcessClusters(int64_t now_ms);

  bool IsBitrateImproving(int probe_bitrate_bps) const;

  void TimeoutStreams(int64_t now_ms);

  Listener* const observer;
  std::unique_ptr<InterArrival> interArrival;
  std::unique_ptr<OveruseEstimator> estimator;
  OveruseDetector detector;
  RateCalculator incomingBitrate;
  bool incomingBitrateInitialized;
  std::vector<int> recentPropagationDeltaMs;
  std::vector<int64_t> recentUpdateTimeMs;
  std::list<Probe> probes;
  size_t totalProbesReceived;
  int64_t firstPacketTimeMs;
  int64_t lastUpdateMs;
  bool umaRecorded;

  Ssrcs ssrcs;
  AimdRateControl remoteRate;

};

}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_HPP
