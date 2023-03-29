/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_

#include "api/network_state_predictor.h"
#include "api/transport/webrtc_key_value_config.h"
#include "modules/congestion_controller/goog_cc/delay_increase_detector_interface.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/experiments/struct_parameters_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <deque>
#include <utility>

namespace webrtc {

struct TrendlineEstimatorSettings {
  static constexpr char kKey[] = "WebRTC-Bwe-TrendlineEstimatorSettings";
  static constexpr unsigned kDefaultTrendlineWindowSize = 10;

  TrendlineEstimatorSettings() = delete;
  explicit TrendlineEstimatorSettings(const WebRtcKeyValueConfig* key_value_config);

  // Sort the packets in the window. Should be redundant,
  // but then almost no cost.
  bool enable_sort = false;

  // Cap the trendline slope based on the minimum delay seen
  // in the beginning_packets and end_packets respectively.
  bool enable_cap = false;
  unsigned beginning_packets = 7;
  unsigned end_packets = 7;
  double cap_uncertainty = 0.0;

  // Size (in packets) of the window.
  unsigned window_size = kDefaultTrendlineWindowSize;

  std::unique_ptr<StructParametersParser> Parser();
};

class TrendlineEstimator : public DelayIncreaseDetectorInterface {
 public:
  TrendlineEstimator(const WebRtcKeyValueConfig* key_value_config,
                     NetworkStatePredictor* network_state_predictor);
  // |window_size| is the number of points required to compute a trend line.
  // |smoothing_coef| controls how much we smooth out the delay before fitting
  // the trend line. |threshold_gain| is used to scale the trendline slope for
  // comparison to the old threshold. Once the old estimator has been removed
  // (or the thresholds been merged into the estimators), we can just set the
  // threshold instead of setting a gain.|network_state_predictor| is used to
  // bettter predict network state.
  TrendlineEstimator(size_t window_size,
                     double smoothing_coef,
                     double threshold_gain,
                     NetworkStatePredictor* network_state_predictor);

  ~TrendlineEstimator() override;

  TrendlineEstimator(const TrendlineEstimator&) = delete;
  TrendlineEstimator& operator=(const TrendlineEstimator&) = delete;

  // Update the estimator with a new sample. The deltas should represent deltas
  // between timestamp groups as defined by the InterArrival class.
  void Update(double recv_delta_ms,
              double send_delta_ms,
              int64_t send_time_ms,
              int64_t arrival_time_ms,
              size_t packet_size,
              bool calculated_deltas) override;

  void UpdateTrendline(double recv_delta_ms,
                       double send_delta_ms,
                       int64_t send_time_ms,
                       int64_t arrival_time_ms,
                       size_t packet_size);

  BandwidthUsage State() const override;

  struct PacketTiming {
    PacketTiming(double arrival_time_ms,
                 double smoothed_delay_ms,
                 double raw_delay_ms)
        : arrival_time_ms(arrival_time_ms),
          smoothed_delay_ms(smoothed_delay_ms),
          raw_delay_ms(raw_delay_ms) {}
    double arrival_time_ms;
    double smoothed_delay_ms;
    double raw_delay_ms;
  };

	DelayIncreaseDetectorInterface::RegressionResult GetTrend() override
	{
		return prev_trend_;
	}

	double GetThreshold() override
	{
		return threshold_;
	}
 private:
  friend class GoogCcStatePrinter;
  void Detect(RegressionResult trend, double ts_delta, int64_t now_ms, double avg_r_squared);

  void UpdateThreshold(double modified_offset, int64_t now_ms);

  // Parameters.
  TrendlineEstimatorSettings settings_;
  const double smoothing_coef_;
  const double threshold_gain_;
  // Used by the existing threshold.
  int num_of_deltas_;
  // Keep the arrival times small by using the change from the first packet.
  int64_t first_arrival_time_ms_;
  // Exponential backoff filtering.
  double accumulated_delay_;
  double smoothed_delay_;
  // Linear least squares regression.
  std::deque<PacketTiming> delay_hist_;
	//
	std::deque<double> r_squared_hist_;

  const double k_up_;
  const double k_down_;
  double overusing_time_threshold_;
  double threshold_;
  double prev_modified_trend_;
  int64_t last_update_ms_;
	DelayIncreaseDetectorInterface::RegressionResult prev_trend_;
  double time_over_using_;
  int overuse_counter_;
	int r_squared_overuse_counter_;
  BandwidthUsage hypothesis_;
  BandwidthUsage hypothesis_predicted_;
  NetworkStatePredictor* network_state_predictor_;
};
}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_
