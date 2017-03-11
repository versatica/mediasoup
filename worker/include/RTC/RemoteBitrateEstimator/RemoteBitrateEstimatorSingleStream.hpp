/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_HPP

#include "common.hpp"
#include "webrtc/base/rate_statistics.h"
#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include <map>
#include <memory>
#include <vector>

namespace RTC {

class RemoteBitrateEstimatorSingleStream : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorSingleStream(RemoteBitrateObserver* observer);
  virtual ~RemoteBitrateEstimatorSingleStream();

  void IncomingPacket(int64_t arrival_time_ms,
                      size_t payload_size,
                      const RtpPacket& packet,
                      const uint8_t* transmissionTimeOffset) override;
  void Process() override;
  int64_t TimeUntilNextProcess() override;
  void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) override;
  void RemoveStream(uint32_t ssrc) override;
  bool LatestEstimate(std::vector<uint32_t>* ssrcs,
                      uint32_t* bitrate_bps) const override;
  void SetMinBitrate(int min_bitrate_bps) override;

 private:
  struct Detector;

  typedef std::map<uint32_t, Detector*> SsrcOveruseEstimatorMap;

  // Triggers a new estimate calculation.
  void UpdateEstimate(int64_t time_now);

  void GetSsrcs(std::vector<uint32_t>* ssrcs) const;

  // Returns |remote_rate_| if the pointed to object exists,
  // otherwise creates it.
  AimdRateControl* GetRemoteRate();

  SsrcOveruseEstimatorMap overuse_detectors_;
  RateStatistics incoming_bitrate_;
  uint32_t last_valid_incoming_bitrate_;
  std::unique_ptr<AimdRateControl> remote_rate_;
  RemoteBitrateObserver* observer_;
  int64_t last_process_time_;
  int64_t process_interval_ms_;
  bool uma_recorded_;

};

}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_SINGLE_STREAM_H_
