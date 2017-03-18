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
  Probe(int64_t sendTimeMs, int64_t recvTimeMs, size_t payloadSize)
      : sendTimeMs(sendTimeMs),
        recvTimeMs(recvTimeMs),
        payloadSize(payloadSize) {}
  int64_t sendTimeMs;
  int64_t recvTimeMs;
  size_t payloadSize;
};

struct Cluster {
  Cluster()
      : sendMeanMs(0.0f),
        recvMeanMs(0.0f),
        meanSize(0),
        count(0),
        numAboveMinDelta(0) {}

  int GetSendBitrateBps() const {
    assert(sendMeanMs > 0.0f);
    return meanSize * 8 * 1000 / sendMeanMs;
  }

  int GetRecvBitrateBps() const {
    assert(recvMeanMs > 0.0f);
    return meanSize * 8 * 1000 / recvMeanMs;
  }

  float sendMeanMs;
  float recvMeanMs;
  // TODO(holmer): Add some variance metric as well?
  size_t meanSize;
  int count;
  int numAboveMinDelta;
};

class RemoteBitrateEstimatorAbsSendTime : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorAbsSendTime(Listener* observer);
  virtual ~RemoteBitrateEstimatorAbsSendTime() {}

  void IncomingPacket(int64_t arrivalTimeMs,
                      size_t payloadSize,
                      const RtpPacket& packet,
                      const uint8_t* absoluteSendTime) override;
  // This class relies on Process() being called periodically (at least once
  // every other second) for streams to be timed out properly.
  void Process() override;
  int64_t TimeUntilNextProcess() override;
  void OnRttUpdate(int64_t avgRttMs, int64_t maxRttMs) override;
  void RemoveStream(uint32_t ssrc) override;
  bool LatestEstimate(std::vector<uint32_t>* ssrcs,
                      uint32_t* bitrateBps) const override;
  void SetMinBitrate(int minBitrateBps) override;

 private:
  typedef std::map<uint32_t, int64_t> Ssrcs;
  enum class ProbeResult { kBitrateUpdated, kNoUpdate };

  static bool IsWithinClusterBounds(int sendDeltaMs,
                                    const Cluster& clusterAggregate);

  static void AddCluster(std::list<Cluster>* clusters, Cluster* cluster);

  void IncomingPacketInfo(int64_t arrivalTimeMs,
                          uint32_t sendTime_24bits,
                          size_t payloadSize,
                          uint32_t ssrc);

  void ComputeClusters(std::list<Cluster>* clusters) const;

  std::list<Cluster>::const_iterator FindBestProbe(
      const std::list<Cluster>& clusters) const;

  // Returns true if a probe which changed the estimate was detected.
  ProbeResult ProcessClusters(int64_t nowMs);

  bool IsBitrateImproving(int probeBitrateBps) const;

  void TimeoutStreams(int64_t nowMs);

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
