/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include "RTC/RtpDataCounter.hpp"
#include <map>
#include <memory>
#include <vector>

namespace RTC {

class RemoteBitrateEstimatorSingleStream : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorSingleStream(Listener* observer);
  virtual ~RemoteBitrateEstimatorSingleStream();

  void IncomingPacket(int64_t arrivalTimeMs,
                      size_t payloadSize,
                      const RtpPacket& packet,
                      const uint8_t* transmissionTimeOffset) override;
  void Process() override;
  int64_t TimeUntilNextProcess() override;
  void OnRttUpdate(int64_t avgRttMs, int64_t maxRttMs) override;
  void RemoveStream(uint32_t ssrc) override;
  bool LatestEstimate(std::vector<uint32_t>* ssrcs,
                      uint32_t* bitrateBps) const override;
  void SetMinBitrate(int minBitrateBps) override;

 private:
  struct Detector;

  typedef std::map<uint32_t, Detector*> SsrcOveruseEstimatorMap;

  // Triggers a new estimate calculation.
  void UpdateEstimate(int64_t timeNow);

  void GetSsrcs(std::vector<uint32_t>* ssrcs) const;

  // Returns |remoteRate| if the pointed to object exists,
  // otherwise creates it.
  AimdRateControl* GetRemoteRate();

  SsrcOveruseEstimatorMap overuseDetectors;
  RateCalculator incomingBitrate;
  uint32_t lastValidIncomingBitrate;
  std::unique_ptr<AimdRateControl> remoteRate;
  Listener* observer;
  int64_t lastProcessTime;
  int64_t processIntervalMs;
  bool umaRecorded;

};

}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP
