/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "OveruseEstimator"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/OveruseEstimator.hpp"
#include "Logger.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

namespace RTC {

enum { kMinFramePeriodHistoryLength = 60 };
enum { kDeltaCounterMax = 1000 };

OveruseEstimator::OveruseEstimator(const OverUseDetectorOptions& options)
    : options(options),
      numOfDeltas(0),
      slope(this->options.initial_slope),
      offset(this->options.initial_offset),
      prevOffset(this->options.initial_offset),
      E(),
      processNoise(),
      avgNoise(this->options.initial_avg_noise),
      varNoise(this->options.initial_var_noise),
      tsDeltaHist() {
  memcpy(this->E, this->options.initial_e, sizeof(this->E));
  memcpy(this->processNoise, this->options.initial_process_noise,
         sizeof(this->processNoise));
}

OveruseEstimator::~OveruseEstimator() {
  this->tsDeltaHist.clear();
}

void OveruseEstimator::Update(int64_t t_delta,
                              double ts_delta,
                              int size_delta,
                              BandwidthUsage current_hypothesis,
                              int64_t now_ms) {
  (void) now_ms;
  const double min_frame_period = UpdateMinFramePeriod(ts_delta);
  const double t_ts_delta = t_delta - ts_delta;
  double fs_delta = size_delta;

  ++this->numOfDeltas;
  if (this->numOfDeltas > kDeltaCounterMax) {
    this->numOfDeltas = kDeltaCounterMax;
  }

  // Update the Kalman filter.
  this->E[0][0] += this->processNoise[0];
  this->E[1][1] += this->processNoise[1];

  if ((current_hypothesis == kBwOverusing && this->offset < this->prevOffset) ||
      (current_hypothesis == kBwUnderusing && this->offset > this->prevOffset)) {
    this->E[1][1] += 10 * this->processNoise[1];
  }

  const double h[2] = {fs_delta, 1.0};
  const double Eh[2] = {this->E[0][0]*h[0] + this->E[0][1]*h[1],
                        this->E[1][0]*h[0] + this->E[1][1]*h[1]};

  const double residual = t_ts_delta - this->slope*h[0] - this->offset;

  const bool in_stable_state = (current_hypothesis == kBwNormal);
  const double max_residual = 3.0 * sqrt(this->varNoise);
  // We try to filter out very late frames. For instance periodic key
  // frames doesn't fit the Gaussian model well.
  if (fabs(residual) < max_residual) {
    UpdateNoiseEstimate(residual, min_frame_period, in_stable_state);
  } else {
    UpdateNoiseEstimate(residual < 0 ? -max_residual : max_residual,
                        min_frame_period, in_stable_state);
  }

  const double denom = this->varNoise + h[0]*Eh[0] + h[1]*Eh[1];

  const double K[2] = {Eh[0] / denom,
                       Eh[1] / denom};

  const double IKh[2][2] = {{1.0 - K[0]*h[0], -K[0]*h[1]},
                            {-K[1]*h[0], 1.0 - K[1]*h[1]}};
  const double e00 = this->E[0][0];
  const double e01 = this->E[0][1];

  // Update state.
  this->E[0][0] = e00 * IKh[0][0] + this->E[1][0] * IKh[0][1];
  this->E[0][1] = e01 * IKh[0][0] + this->E[1][1] * IKh[0][1];
  this->E[1][0] = e00 * IKh[1][0] + this->E[1][0] * IKh[1][1];
  this->E[1][1] = e01 * IKh[1][0] + this->E[1][1] * IKh[1][1];

  // The covariance matrix must be positive semi-definite.
  bool positive_semi_definite = this->E[0][0] + this->E[1][1] >= 0 &&
      this->E[0][0] * this->E[1][1] - this->E[0][1] * this->E[1][0] >= 0 && this->E[0][0] >= 0;
  MS_ASSERT(positive_semi_definite, "'positive_semi_definite' missing");
  if (!positive_semi_definite) {
    MS_ERROR("The over-use estimator's covariance matrix is no longer semi-definite");
  }

  this->slope = this->slope + K[0] * residual;
  this->prevOffset = this->offset;
  this->offset = this->offset + K[1] * residual;
}

double OveruseEstimator::UpdateMinFramePeriod(double ts_delta) {
  double min_frame_period = ts_delta;
  if (this->tsDeltaHist.size() >= kMinFramePeriodHistoryLength) {
    this->tsDeltaHist.pop_front();
  }
  for (const double old_ts_delta : this->tsDeltaHist) {
    min_frame_period = std::min(old_ts_delta, min_frame_period);
  }
  this->tsDeltaHist.push_back(ts_delta);
  return min_frame_period;
}

void OveruseEstimator::UpdateNoiseEstimate(double residual,
                                           double ts_delta,
                                           bool stable_state) {
  if (!stable_state) {
    return;
  }
  // Faster filter during startup to faster adapt to the jitter level
  // of the network. |alpha| is tuned for 30 frames per second, but is scaled
  // according to |ts_delta|.
  double alpha = 0.01;
  if (this->numOfDeltas > 10*30) {
    alpha = 0.002;
  }
  // Only update the noise estimate if we're not over-using. |beta| is a
  // function of alpha and the time delta since the previous update.
  const double beta = pow(1 - alpha, ts_delta * 30.0 / 1000.0);
  this->avgNoise = beta * this->avgNoise
              + (1 - beta) * residual;
  this->varNoise = beta * this->varNoise
              + (1 - beta) * (this->avgNoise - residual) * (this->avgNoise - residual);
  if (this->varNoise < 1) {
    this->varNoise = 1;
  }
}
}  // namespace RTC
