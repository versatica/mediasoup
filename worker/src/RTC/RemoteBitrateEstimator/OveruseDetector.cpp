/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "OveruseDetector"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/OveruseDetector.hpp"
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <string>


namespace RTC {

// (jmillan) disable the experiment until we know how to use the threshold values.
/*
const char kAdaptiveThresholdExperiment[] = "WebRTC-AdaptiveBweThreshold";
const char kEnabledPrefix[] = "Enabled";
const size_t kEnabledPrefixLength = sizeof(kEnabledPrefix) - 1;
const char kDisabledPrefix[] = "Disabled";
const size_t kDisabledPrefixLength = sizeof(kDisabledPrefix) - 1;
*/

const double kMaxAdaptOffsetMs = 15.0;
const double kOverUsingTimeThreshold = 10;
const int kMinNumDeltas = 60;

// (jmillan) disable the experiment until we know how to use the threshold values.
bool AdaptiveThresholdExperimentIsDisabled() {
  return true;
}

bool ReadExperimentConstants(double* k_up, double* k_down) {
  (void) k_up;
  (void) k_down;
  return false;
}

/*
bool AdaptiveThresholdExperimentIsDisabled() {
  std::string experiment_string =
      webrtc::field_trial::FindFullName(kAdaptiveThresholdExperiment);
  const size_t kMinExperimentLength = kDisabledPrefixLength;
  if (experiment_string.length() < kMinExperimentLength)
    return false;
  return experiment_string.substr(0, kDisabledPrefixLength) == kDisabledPrefix;
}
*/

// Gets thresholds from the experiment name following the format
// "WebRTC-AdaptiveBweThreshold/Enabled-0.5,0.002/".
/*
bool ReadExperimentConstants(double* k_up, double* k_down) {
  std::string experiment_string =
      webrtc::field_trial::FindFullName(kAdaptiveThresholdExperiment);
  const size_t kMinExperimentLength = kEnabledPrefixLength + 3;
  if (experiment_string.length() < kMinExperimentLength ||
      experiment_string.substr(0, kEnabledPrefixLength) != kEnabledPrefix)
    return false;
  return sscanf(experiment_string.substr(kEnabledPrefixLength + 1).c_str(),
                "%lf,%lf", k_up, k_down) == 2;
}
*/

OveruseDetector::OveruseDetector() :
    // Experiment is on by default, but can be disabled with finch by setting
    // the field trial string to "WebRTC-AdaptiveBweThreshold/Disabled/".
    inExperiment(!AdaptiveThresholdExperimentIsDisabled()),
    kUp(0.0087),
    kDown(0.039),
    overusingTimeThreshold(100),
    threshold(12.5),
    lastUpdateMs(-1),
    prevOffset(0.0),
    timeOverUsing(-1),
    overuseCounter(0),
    hypothesis(kBwNormal) {
  if (!AdaptiveThresholdExperimentIsDisabled())
    InitializeExperiment();
}

OveruseDetector::~OveruseDetector() {}

BandwidthUsage OveruseDetector::State() const {
  return this->hypothesis;
}

BandwidthUsage OveruseDetector::Detect(double offset,
                                       double ts_delta,
                                       int num_of_deltas,
                                       int64_t now_ms) {
  if (num_of_deltas < 2) {
    return kBwNormal;
  }
  const double T = std::min(num_of_deltas, kMinNumDeltas) * offset;
  if (T > this->threshold) {
    if (this->timeOverUsing == -1) {
      // Initialize the timer. Assume that we've been
      // over-using half of the time since the previous
      // sample.
      this->timeOverUsing = ts_delta / 2;
    } else {
      // Increment timer
      this->timeOverUsing += ts_delta;
    }
    this->overuseCounter++;
    if (this->timeOverUsing > this->overusingTimeThreshold && this->overuseCounter > 1) {
      if (offset >= this->prevOffset) {
        this->timeOverUsing = 0;
        this->overuseCounter = 0;
        this->hypothesis = kBwOverusing;
      }
    }
  } else if (T < -this->threshold) {
    this->timeOverUsing = -1;
    this->overuseCounter = 0;
    this->hypothesis = kBwUnderusing;
  } else {
    this->timeOverUsing = -1;
    this->overuseCounter = 0;
    this->hypothesis = kBwNormal;
  }
  this->prevOffset = offset;

  UpdateThreshold(T, now_ms);

  return this->hypothesis;
}

void OveruseDetector::UpdateThreshold(double modified_offset, int64_t now_ms) {
  if (!this->inExperiment)
    return;

  if (this->lastUpdateMs == -1)
    this->lastUpdateMs = now_ms;

  if (fabs(modified_offset) > this->threshold + kMaxAdaptOffsetMs) {
    // Avoid adapting the threshold to big latency spikes, caused e.g.,
    // by a sudden capacity drop.
    this->lastUpdateMs = now_ms;
    return;
  }

  const double k = fabs(modified_offset) < this->threshold ? this->kDown : this->kUp;
  const int64_t kMaxTimeDeltaMs = 100;
  int64_t time_delta_ms = std::min(now_ms - this->lastUpdateMs, kMaxTimeDeltaMs);
  this->threshold +=
      k * (fabs(modified_offset) - this->threshold) * time_delta_ms;

  const double kMinThreshold = 6;
  const double kMaxThreshold = 600;
  this->threshold = std::min(std::max(this->threshold, kMinThreshold), kMaxThreshold);

  this->lastUpdateMs = now_ms;
}

void OveruseDetector::InitializeExperiment() {
  //MS_DASSERT(this->inExperiment);
  double k_up = 0.0;
  double k_down = 0.0;
  this->overusingTimeThreshold = kOverUsingTimeThreshold;
  if (ReadExperimentConstants(&k_up, &k_down)) {
    this->kUp = k_up;
    this->kDown = k_down;
  }
}
}  // namespace RTC
