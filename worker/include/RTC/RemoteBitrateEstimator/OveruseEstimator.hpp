/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_ESTIMATOR_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/BweDefines.hpp"
#include <deque>

namespace RTC {

// (jmillan) borrowed from webrtc/common_types.h
//
// Bandwidth over-use detector options.  These are used to drive
// experimentation with bandwidth estimation parameters.
// See modules/remote_bitrate_estimator/overuse_detector.h
// TODO(terelius): This is only used in overuse_estimator.cc, and only in the
// default constructed state. Can we move the relevant variables into that
// class and delete this? See also disabled warning at line 27
struct OverUseDetectorOptions {
  OverUseDetectorOptions()
      : initial_slope(8.0 / 512.0),
        initial_offset(0),
        initial_e(),
        initial_process_noise(),
        initial_avg_noise(0.0),
        initial_var_noise(50) {
    initial_e[0][0] = 100;
    initial_e[1][1] = 1e-1;
    initial_e[0][1] = initial_e[1][0] = 0;
    initial_process_noise[0] = 1e-13;
    initial_process_noise[1] = 1e-3;
  }
  double initial_slope;
  double initial_offset;
  double initial_e[2][2];
  double initial_process_noise[2];
  double initial_avg_noise;
  double initial_var_noise;
};

class OveruseEstimator {
 public:
  explicit OveruseEstimator(const OverUseDetectorOptions& options);
  ~OveruseEstimator();

  // Update the estimator with a new sample. The deltas should represent deltas
  // between timestamp groups as defined by the InterArrival class.
  // |current_hypothesis| should be the hypothesis of the over-use detector at
  // this time.
  void Update(int64_t t_delta,
              double ts_delta,
              int size_delta,
              BandwidthUsage current_hypothesis,
              int64_t now_ms);

  // Returns the estimated noise/jitter variance in ms^2.
  double var_noise() const {
    return var_noise_;
  }

  // Returns the estimated inter-arrival time delta offset in ms.
  double offset() const {
    return offset_;
  }

  // Returns the number of deltas which the current over-use estimator state is
  // based on.
  unsigned int num_of_deltas() const {
    return num_of_deltas_;
  }

 private:
  double UpdateMinFramePeriod(double ts_delta);
  void UpdateNoiseEstimate(double residual, double ts_delta, bool stable_state);

  // Must be first member variable. Cannot be const because we need to be
  // copyable.
  OverUseDetectorOptions options_;
  uint16_t num_of_deltas_;
  double slope_;
  double offset_;
  double prev_offset_;
  double E_[2][2];
  double process_noise_[2];
  double avg_noise_;
  double var_noise_;
  std::deque<double> ts_delta_hist_;

};
}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_OVERUSE_ESTIMATOR_H_
