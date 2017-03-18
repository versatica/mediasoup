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
  this->currentBitrateBps = start_bitrate_bps;
  this->bitrateIsInitialized = true;
}

void AimdRateControl::SetMinBitrate(int min_bitrate_bps) {
  this->minConfiguredBitrateBps = min_bitrate_bps;
  this->currentBitrateBps = std::max<int>(min_bitrate_bps, this->currentBitrateBps);
}

bool AimdRateControl::ValidEstimate() const {
  return this->bitrateIsInitialized;
}

int64_t AimdRateControl::GetFeedbackInterval() const {
  // Estimate how often we can send RTCP if we allocate up to 5% of bandwidth
  // to feedback.
  static const int kRtcpSize = 80;
  int64_t interval = static_cast<int64_t>(
      kRtcpSize * 8.0 * 1000.0 / (0.05 * this->currentBitrateBps) + 0.5);
  const int64_t kMinFeedbackIntervalMs = 200;
  return std::min(std::max(interval, kMinFeedbackIntervalMs),
                  kMaxFeedbackIntervalMs);
}

bool AimdRateControl::TimeToReduceFurther(int64_t time_now,
                                          uint32_t incoming_bitrate_bps) const {
  const int64_t bitrate_reduction_interval =
      std::max<int64_t>(std::min<int64_t>(this->rtt, 200), 10);
  if (time_now - this->timeLastBitrateChange >= bitrate_reduction_interval) {
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
  return this->currentBitrateBps;
}

uint32_t AimdRateControl::UpdateBandwidthEstimate(int64_t now_ms) {
  this->currentBitrateBps = ChangeBitrate(
      this->currentBitrateBps,
      this->currentInput.incoming_bitrate, now_ms);
  return this->currentBitrateBps;
}

void AimdRateControl::SetRtt(int64_t rtt) {
  this->rtt = rtt;
}

void AimdRateControl::Update(const RateControlInput* input, int64_t now_ms) {
  MS_ASSERT(input, "'input' missing");

  // Set the initial bit rate value to what we're receiving the first half
  // second.
  if (!this->bitrateIsInitialized) {
    const int64_t kInitializationTimeMs = 5000;
    //MS_DASSERT(kBitrateWindowMs <= kInitializationTimeMs);
    if (this->timeFirstIncomingEstimate < 0) {
      if (input->incoming_bitrate)
        this->timeFirstIncomingEstimate = now_ms;
    } else if (now_ms - this->timeFirstIncomingEstimate > kInitializationTimeMs &&
               input->incoming_bitrate) {
      this->currentBitrateBps = input->incoming_bitrate;
      this->bitrateIsInitialized = true;
    }
  }

  if (this->updated && this->currentInput.bw_state == kBwOverusing) {
    // Only update delay factor and incoming bit rate. We always want to react
    // on an over-use.
    this->currentInput.noise_var = input->noise_var;
    this->currentInput.incoming_bitrate = input->incoming_bitrate;
  } else {
    this->updated = true;
    this->currentInput = *input;
  }
}

void AimdRateControl::SetEstimate(int bitrate_bps, int64_t now_ms) {
  this->updated = true;
  this->bitrateIsInitialized = true;
  this->currentBitrateBps = ClampBitrate(bitrate_bps, bitrate_bps);
  this->timeLastBitrateChange = now_ms;
}

int AimdRateControl::GetNearMaxIncreaseRateBps() const {
  //MS_DASSERT(this->currentBitrateBps > 0);
  double bits_per_frame = static_cast<double>(this->currentBitrateBps) / 30.0;
  double packets_per_frame = std::ceil(bits_per_frame / (8.0 * 1200.0));
  double avg_packet_size_bits = bits_per_frame / packets_per_frame;
  // Approximate the over-use estimator delay to 100 ms.
  const int64_t response_time = this->inExperiment ? (this->rtt + 100) * 2 : this->rtt + 100;

  constexpr double kMinIncreaseRateBps = 4000;
  return static_cast<int>(std::max(
      kMinIncreaseRateBps, (avg_packet_size_bits * 1000) / response_time));
}

int AimdRateControl::GetLastBitrateDecreaseBps() const {
  return this->lastDecrease;
}

uint32_t AimdRateControl::ChangeBitrate(uint32_t new_bitrate_bps,
                                        uint32_t incoming_bitrate_bps,
                                        int64_t now_ms) {
  if (!this->updated) {
    return this->currentBitrateBps;
  }
  // An over-use should always trigger us to reduce the bitrate, even though
  // we have not yet established our first estimate. By acting on the over-use,
  // we will end up with a valid estimate.
  if (!this->bitrateIsInitialized && this->currentInput.bw_state != kBwOverusing)
    return this->currentBitrateBps;
  this->updated = false;
  ChangeState(this->currentInput, now_ms);
  // Calculated here because it's used in multiple places.
  const float incoming_bitrate_kbps = incoming_bitrate_bps / 1000.0f;
  // Calculate the max bit rate std dev given the normalized
  // variance and the current incoming bit rate.
  const float std_max_bit_rate = sqrt(this->varMaxBitrateKbps *
                                      this->avgMaxBitrateKbps);
  switch (this->rateControlState) {
    case kRcHold:
      break;

    case kRcIncrease:
      if (this->avgMaxBitrateKbps >= 0 &&
          incoming_bitrate_kbps >
              this->avgMaxBitrateKbps + 3 * std_max_bit_rate) {
        ChangeRegion(kRcMaxUnknown);
        this->avgMaxBitrateKbps = -1.0;
      }
      if (this->rateControlRegion == kRcNearMax) {
        uint32_t additive_increase_bps =
            AdditiveRateIncrease(now_ms, this->timeLastBitrateChange);
        new_bitrate_bps += additive_increase_bps;
      } else {
        uint32_t multiplicative_increase_bps = MultiplicativeRateIncrease(
            now_ms, this->timeLastBitrateChange, new_bitrate_bps);
        new_bitrate_bps += multiplicative_increase_bps;
      }

      this->timeLastBitrateChange = now_ms;
      break;

    case kRcDecrease:
      this->bitrateIsInitialized = true;
      // Set bit rate to something slightly lower than max
      // to get rid of any self-induced delay.
      new_bitrate_bps =
          static_cast<uint32_t>(this->beta * incoming_bitrate_bps + 0.5);
      if (new_bitrate_bps > this->currentBitrateBps) {
        // Avoid increasing the rate when over-using.
        if (this->rateControlRegion != kRcMaxUnknown) {
          new_bitrate_bps = static_cast<uint32_t>(
              this->beta * this->avgMaxBitrateKbps * 1000 + 0.5f);
        }
        new_bitrate_bps = std::min(new_bitrate_bps, this->currentBitrateBps);
      }
      ChangeRegion(kRcNearMax);

      if (incoming_bitrate_bps < this->currentBitrateBps) {
        this->lastDecrease = int(this->currentBitrateBps - new_bitrate_bps);
      }
      if (incoming_bitrate_kbps <
          this->avgMaxBitrateKbps - 3 * std_max_bit_rate) {
        this->avgMaxBitrateKbps = -1.0f;
      }

      UpdateMaxBitRateEstimate(incoming_bitrate_kbps);
      // Stay on hold until the pipes are cleared.
      ChangeState(kRcHold);
      this->timeLastBitrateChange = now_ms;
      break;

    default:
      MS_ASSERT(false, "invalid 'this->rateControlState' value");
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
  if (new_bitrate_bps > this->currentBitrateBps &&
      new_bitrate_bps > max_bitrate_bps) {
    new_bitrate_bps = std::max(this->currentBitrateBps, max_bitrate_bps);
  }
  new_bitrate_bps = std::max(new_bitrate_bps, this->minConfiguredBitrateBps);
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
  if (this->avgMaxBitrateKbps == -1.0f) {
    this->avgMaxBitrateKbps = incoming_bitrate_kbps;
  } else {
    this->avgMaxBitrateKbps = (1 - alpha) * this->avgMaxBitrateKbps +
        alpha * incoming_bitrate_kbps;
  }
  // Estimate the max bit rate variance and normalize the variance
  // with the average max bit rate.
  const float norm = std::max(this->avgMaxBitrateKbps, 1.0f);
  this->varMaxBitrateKbps = (1 - alpha) * this->varMaxBitrateKbps +
      alpha * (this->avgMaxBitrateKbps - incoming_bitrate_kbps) *
          (this->avgMaxBitrateKbps - incoming_bitrate_kbps) / norm;
  // 0.4 ~= 14 kbit/s at 500 kbit/s
  if (this->varMaxBitrateKbps < 0.4f) {
    this->varMaxBitrateKbps = 0.4f;
  }
  // 2.5f ~= 35 kbit/s at 500 kbit/s
  if (this->varMaxBitrateKbps > 2.5f) {
    this->varMaxBitrateKbps = 2.5f;
  }
}

void AimdRateControl::ChangeState(const RateControlInput& input,
                                  int64_t now_ms) {
  (void) input;
  switch (this->currentInput.bw_state) {
    case kBwNormal:
      if (this->rateControlState == kRcHold) {
        this->timeLastBitrateChange = now_ms;
        ChangeState(kRcIncrease);
      }
      break;
    case kBwOverusing:
      if (this->rateControlState != kRcDecrease) {
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
  this->rateControlRegion = region;
}

void AimdRateControl::ChangeState(RateControlState new_state) {
  this->rateControlState = new_state;
}
}  // namespace RTC
