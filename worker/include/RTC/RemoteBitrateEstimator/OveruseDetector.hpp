/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_DETECTOR_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_DETECTOR_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/RateControlRegion.hpp"
#include "RTC/RemoteBitrateEstimator/BandwidthUsage.hpp"
#include <list>

namespace RTC {

bool AdaptiveThresholdExperimentIsDisabled();

class OveruseDetector {
 public:
  OveruseDetector();
  virtual ~OveruseDetector();

  // Update the detection state based on the estimated inter-arrival time delta
  // offset. |timestamp_delta| is the delta between the last timestamp which the
  // estimated offset is based on and the last timestamp on which the last
  // offset was based on, representing the time between detector updates.
  // |num_of_deltas| is the number of deltas the offset estimate is based on.
  // Returns the state after the detection update.
  BandwidthUsage Detect(double offset,
                        double timestamp_delta,
                        int num_of_deltas,
                        int64_t now_ms);

  // Returns the current detector state.
  BandwidthUsage State() const;

 private:
  void UpdateThreshold(double modified_offset, int64_t now_ms);
  void InitializeExperiment();

  bool inExperiment;
  double kUp;
  double kDown;
  double overusingTimeThreshold;
  double threshold;
  int64_t lastUpdateMs;
  double prevOffset;
  double timeOverUsing;
  int overuseCounter;
  BandwidthUsage hypothesis;

};
}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_DETECTOR_HPP
