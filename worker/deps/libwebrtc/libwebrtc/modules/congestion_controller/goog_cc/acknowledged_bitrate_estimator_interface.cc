/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#define MS_CLASS "webrtc::AcknowledgedBitrateEstimator"

#include "modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator_interface.h"

#include <algorithm>

#include "api/units/time_delta.h"
#include "modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.h"
#include "modules/congestion_controller/goog_cc/robust_throughput_estimator.h"
#include "api/transport/webrtc_key_value_config.h"
#include "Logger.hpp"
// #include "rtc_base/logging.h"

namespace webrtc {

constexpr char RobustThroughputEstimatorSettings::kKey[];

RobustThroughputEstimatorSettings::RobustThroughputEstimatorSettings(
    const WebRtcKeyValueConfig* key_value_config) {
  Parser()->Parse(
      key_value_config->Lookup(RobustThroughputEstimatorSettings::kKey));
  if (window_packets < 10 || 1000 < window_packets) {
    MS_WARN_TAG(bwe, "window size must be between 10 and 1000 packets");
    window_packets = 20;
  }
  if (max_window_packets < 10 || 1000 < max_window_packets) {
    MS_WARN_TAG(bwe, "max window size must be between 10 and 1000 packets");
    max_window_packets = 500;
  }
  max_window_packets = std::max(max_window_packets, window_packets);

  if (required_packets < 10 || 1000 < required_packets) {
    MS_WARN_TAG(bwe, "required number of initial packets must be between "
                           "10 and 1000 packets");
    required_packets = 10;
  }
  required_packets = std::min(required_packets, window_packets);

  if (min_window_duration < TimeDelta::Millis<100>() ||
      TimeDelta::Millis<3000>() < min_window_duration) {
    MS_WARN_TAG(bwe, "window duration must be between 100 and 3000 ms");
    min_window_duration = TimeDelta::Millis<750>();
  }
  if (max_window_duration < TimeDelta::Seconds<1>() ||
      TimeDelta::Seconds<15>() < max_window_duration) {
    MS_WARN_TAG(bwe, "max window duration must be between 1 and 15 seconds");
    max_window_duration = TimeDelta::Seconds<5>();
  }
  min_window_duration = std::min(min_window_duration, max_window_duration);

  if (unacked_weight < 0.0 || 1.0 < unacked_weight) {
    MS_WARN_TAG(bwe, "weight for prior unacked size must be between 0 and 1");
    unacked_weight = 1.0;
  }
}

std::unique_ptr<StructParametersParser>
RobustThroughputEstimatorSettings::Parser() {
  return StructParametersParser::Create(
      "enabled", &enabled,                          //
      "window_packets", &window_packets,            //
      "max_window_packets", &max_window_packets,    //
      "window_duration", &min_window_duration,      //
      "max_window_duration", &max_window_duration,  //
      "required_packets", &required_packets,        //
      "unacked_weight", &unacked_weight);
}

AcknowledgedBitrateEstimatorInterface::
    ~AcknowledgedBitrateEstimatorInterface() {}

std::unique_ptr<AcknowledgedBitrateEstimatorInterface>
AcknowledgedBitrateEstimatorInterface::Create(
    const WebRtcKeyValueConfig* key_value_config) {
  RobustThroughputEstimatorSettings simplified_estimator_settings(
      key_value_config);
  if (simplified_estimator_settings.enabled) {
    return absl::make_unique<RobustThroughputEstimator>(
        simplified_estimator_settings);
  }
  return absl::make_unique<AcknowledgedBitrateEstimator>(key_value_config);
}

}  // namespace webrtc
