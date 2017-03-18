/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#define MS_CLASS "AimdRateControl"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/AimdRateControl.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimator.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>

static constexpr int64_t kMaxFeedbackIntervalMs = 1000;

namespace RTC
{

void AimdRateControl::SetStartBitrate(int start_bitrate_bps) {
  this->current_bitrate_bps_ = start_bitrate_bps;
  this->bitrate_is_initialized_ = true;
}

void AimdRateControl::SetMinBitrate(int min_bitrate_bps) {
  this->min_configured_bitrate_bps_ = min_bitrate_bps;
  this->current_bitrate_bps_ = std::max<int>(min_bitrate_bps, this->current_bitrate_bps_);
}

bool AimdRateControl::ValidEstimate() const {
  return this->bitrate_is_initialized_;
}

int64_t AimdRateControl::GetFeedbackInterval() const {
  // Estimate how often we can send RTCP if we allocate up to 5% of bandwidth
  // to feedback.
  static const int kRtcpSize = 80;
  int64_t interval = static_cast<int64_t>(
      kRtcpSize * 8.0 * 1000.0 / (0.05 * this->current_bitrate_bps_) + 0.5);
  const int64_t kMinFeedbackIntervalMs = 200;
  return std::min(std::max(interval, kMinFeedbackIntervalMs),
                  kMaxFeedbackIntervalMs);
}

bool AimdRateControl::TimeToReduceFurther(int64_t time_now,
                                          uint32_t incoming_bitrate_bps) const {
  const int64_t bitrate_reduction_interval =
      std::max<int64_t>(std::min<int64_t>(this->rtt_, 200), 10);
  if (time_now - this->time_last_bitrate_change_ >= bitrate_reduction_interval) {
    return true;
  }
  if (ValidEstimate()) {
    // TODO(terelius/holmer): Investigate consequences of increasing
    // the threshold to 0.95 * LatestEstimate().
    const uint32_t threshold = static_cast<uint32_t> (0.5 * LatestEstimate());
    return incoming_bitrate_bps < threshold;
  }
  return false;
}

uint32_t AimdRateControl::LatestEstimate() const {
  return this->current_bitrate_bps_;
}

uint32_t AimdRateControl::UpdateBandwidthEstimate(int64_t now_ms) {
  this->current_bitrate_bps_ = ChangeBitrate(
      this->current_bitrate_bps_,
      this->current_input_.incoming_bitrate, now_ms);
  return this->current_bitrate_bps_;
}

void AimdRateControl::SetRtt(int64_t rtt) {
  this->rtt_ = rtt;
}

void AimdRateControl::Update(const RateControlInput* input, int64_t now_ms) {
  MS_ASSERT(input, "'input' missing");

  // Set the initial bit rate value to what we're receiving the first half
  // second.
  if (!this->bitrate_is_initialized_) {
    const int64_t kInitializationTimeMs = 5000;
    //MS_DASSERT(kBitrateWindowMs <= kInitializationTimeMs);
    if (this->time_first_incoming_estimate_ < 0) {
      if (input->incoming_bitrate)
        this->time_first_incoming_estimate_ = now_ms;
    } else if (now_ms - this->time_first_incoming_estimate_ > kInitializationTimeMs &&
               input->incoming_bitrate) {
      this->current_bitrate_bps_ = input->incoming_bitrate;
      this->bitrate_is_initialized_ = true;
    }
  }

  if (this->updated_ && this->current_input_.bw_state == kBwOverusing) {
    // Only update delay factor and incoming bit rate. We always want to react
    // on an over-use.
    this->current_input_.noise_var = input->noise_var;
    this->current_input_.incoming_bitrate = input->incoming_bitrate;
  } else {
    this->updated_ = true;
    this->current_input_ = *input;
  }
}

void AimdRateControl::SetEstimate(int bitrate_bps, int64_t now_ms) {
  this->updated_ = true;
  this->bitrate_is_initialized_ = true;
  this->current_bitrate_bps_ = ClampBitrate(bitrate_bps, bitrate_bps);
  this->time_last_bitrate_change_ = now_ms;
}

int AimdRateControl::GetNearMaxIncreaseRateBps() const {
  //MS_DASSERT(this->current_bitrate_bps_ > 0);
  double bits_per_frame = static_cast<double>(this->current_bitrate_bps_) / 30.0;
  double packets_per_frame = std::ceil(bits_per_frame / (8.0 * 1200.0));
  double avg_packet_size_bits = bits_per_frame / packets_per_frame;
  // Approximate the over-use estimator delay to 100 ms.
  const int64_t response_time = this->in_experiment_ ? (this->rtt_ + 100) * 2 : this->rtt_ + 100;

  constexpr double kMinIncreaseRateBps = 4000;
  return static_cast<int>(std::max(
      kMinIncreaseRateBps, (avg_packet_size_bits * 1000) / response_time));
}

int AimdRateControl::GetLastBitrateDecreaseBps() const {
  return this->last_decrease_;
}

uint32_t AimdRateControl::ChangeBitrate(uint32_t new_bitrate_bps,
                                        uint32_t incoming_bitrate_bps,
                                        int64_t now_ms) {
  if (!this->updated_) {
    return this->current_bitrate_bps_;
  }
  // An over-use should always trigger us to reduce the bitrate, even though
  // we have not yet established our first estimate. By acting on the over-use,
  // we will end up with a valid estimate.
  if (!this->bitrate_is_initialized_ && this->current_input_.bw_state != kBwOverusing)
    return this->current_bitrate_bps_;
  this->updated_ = false;
  ChangeState(this->current_input_, now_ms);
  // Calculated here because it's used in multiple places.
  const float incoming_bitrate_kbps = incoming_bitrate_bps / 1000.0f;
  // Calculate the max bit rate std dev given the normalized
  // variance and the current incoming bit rate.
  const float std_max_bit_rate = sqrt(this->var_max_bitrate_kbps_ *
                                      this->avg_max_bitrate_kbps_);
  switch (this->rate_control_state_) {
    case kRcHold:
      break;

    case kRcIncrease:
      if (this->avg_max_bitrate_kbps_ >= 0 &&
          incoming_bitrate_kbps >
              this->avg_max_bitrate_kbps_ + 3 * std_max_bit_rate) {
        ChangeRegion(kRcMaxUnknown);
        this->avg_max_bitrate_kbps_ = -1.0;
      }
      if (this->rate_control_region_ == kRcNearMax) {
        uint32_t additive_increase_bps =
            AdditiveRateIncrease(now_ms, this->time_last_bitrate_change_);
        new_bitrate_bps += additive_increase_bps;
      } else {
        uint32_t multiplicative_increase_bps = MultiplicativeRateIncrease(
            now_ms, this->time_last_bitrate_change_, new_bitrate_bps);
        new_bitrate_bps += multiplicative_increase_bps;
      }

      this->time_last_bitrate_change_ = now_ms;
      break;

    case kRcDecrease:
      this->bitrate_is_initialized_ = true;
      // Set bit rate to something slightly lower than max
      // to get rid of any self-induced delay.
      new_bitrate_bps =
          static_cast<uint32_t>(this->beta_ * incoming_bitrate_bps + 0.5);
      if (new_bitrate_bps > this->current_bitrate_bps_) {
        // Avoid increasing the rate when over-using.
        if (this->rate_control_region_ != kRcMaxUnknown) {
          new_bitrate_bps = static_cast<uint32_t>(
              this->beta_ * this->avg_max_bitrate_kbps_ * 1000 + 0.5f);
        }
        new_bitrate_bps = std::min(new_bitrate_bps, this->current_bitrate_bps_);
      }
      ChangeRegion(kRcNearMax);

      if (incoming_bitrate_bps < this->current_bitrate_bps_) {
        this->last_decrease_ = int(this->current_bitrate_bps_ - new_bitrate_bps);
      }
      if (incoming_bitrate_kbps <
          this->avg_max_bitrate_kbps_ - 3 * std_max_bit_rate) {
        this->avg_max_bitrate_kbps_ = -1.0f;
      }

      UpdateMaxBitRateEstimate(incoming_bitrate_kbps);
      // Stay on hold until the pipes are cleared.
      ChangeState(kRcHold);
      this->time_last_bitrate_change_ = now_ms;
      break;

    default:
      MS_ASSERT(false, "invalid 'this->rate_control_state_' value");
  }
  return ClampBitrate(new_bitrate_bps, incoming_bitrate_bps);
}

uint32_t AimdRateControl::ClampBitrate(uint32_t new_bitrate_bps,
                                       uint32_t incoming_bitrate_bps) const {
  // Don't change the bit rate if the send side is too far off.
  // We allow a bit more lag at very low rates to not too easily get stuck if
  // the encoder produces uneven outputs.
  const uint32_t max_bitrate_bps =
      static_cast<uint32_t>(1.5f * incoming_bitrate_bps) + 10000;
  if (new_bitrate_bps > this->current_bitrate_bps_ &&
      new_bitrate_bps > max_bitrate_bps) {
    new_bitrate_bps = std::max(this->current_bitrate_bps_, max_bitrate_bps);
  }
  new_bitrate_bps = std::max(new_bitrate_bps, this->min_configured_bitrate_bps_);
  return new_bitrate_bps;
}

uint32_t AimdRateControl::MultiplicativeRateIncrease(
    int64_t now_ms, int64_t last_ms, uint32_t current_bitrate_bps) const {
  double alpha = 1.08;
  if (last_ms > -1) {
    int time_since_last_update_ms = std::min(static_cast<int>(now_ms - last_ms),
                                             1000);
    alpha = pow(alpha,  time_since_last_update_ms / 1000.0);
  }
  uint32_t multiplicative_increase_bps = std::max(
      current_bitrate_bps * (alpha - 1.0), 1000.0);
  return multiplicative_increase_bps;
}

uint32_t AimdRateControl::AdditiveRateIncrease(int64_t now_ms,
                                               int64_t last_ms) const {
  return static_cast<uint32_t>((now_ms - last_ms) *
                               GetNearMaxIncreaseRateBps() / 1000);
}

void AimdRateControl::UpdateMaxBitRateEstimate(float incoming_bitrate_kbps) {
  const float alpha = 0.05f;
  if (this->avg_max_bitrate_kbps_ == -1.0f) {
    this->avg_max_bitrate_kbps_ = incoming_bitrate_kbps;
  } else {
    this->avg_max_bitrate_kbps_ = (1 - alpha) * this->avg_max_bitrate_kbps_ +
        alpha * incoming_bitrate_kbps;
  }
  // Estimate the max bit rate variance and normalize the variance
  // with the average max bit rate.
  const float norm = std::max(this->avg_max_bitrate_kbps_, 1.0f);
  this->var_max_bitrate_kbps_ = (1 - alpha) * this->var_max_bitrate_kbps_ +
      alpha * (this->avg_max_bitrate_kbps_ - incoming_bitrate_kbps) *
          (this->avg_max_bitrate_kbps_ - incoming_bitrate_kbps) / norm;
  // 0.4 ~= 14 kbit/s at 500 kbit/s
  if (this->var_max_bitrate_kbps_ < 0.4f) {
    this->var_max_bitrate_kbps_ = 0.4f;
  }
  // 2.5f ~= 35 kbit/s at 500 kbit/s
  if (this->var_max_bitrate_kbps_ > 2.5f) {
    this->var_max_bitrate_kbps_ = 2.5f;
  }
}

void AimdRateControl::ChangeState(const RateControlInput& input,
                                  int64_t now_ms) {
  (void) input;
  switch (this->current_input_.bw_state) {
    case kBwNormal:
      if (this->rate_control_state_ == kRcHold) {
        this->time_last_bitrate_change_ = now_ms;
        ChangeState(kRcIncrease);
      }
      break;
    case kBwOverusing:
      if (this->rate_control_state_ != kRcDecrease) {
        ChangeState(kRcDecrease);
      }
      break;
    case kBwUnderusing:
      ChangeState(kRcHold);
      break;
    default:
      MS_ASSERT(false, "invalid 'RateControlInput::bw_state' value");
  }
}

void AimdRateControl::ChangeRegion(RateControlRegion region) {
  this->rate_control_region_ = region;
}

void AimdRateControl::ChangeState(RateControlState new_state) {
  this->rate_control_state_ = new_state;
}
}  // namespace RTC
