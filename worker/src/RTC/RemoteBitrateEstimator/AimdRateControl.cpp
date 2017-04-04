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

	int64_t AimdRateControl::GetFeedbackInterval() const
	{
		MS_TRACE();

		// Estimate how often we can send RTCP if we allocate up to 5% of bandwidth
		// to feedback.
		static const int kRtcpSize = 80;
		int64_t interval = static_cast<int64_t>(kRtcpSize * 8.0 * 1000.0 / (0.05 * this->currentBitrateBps) + 0.5);
		const int64_t kMinFeedbackIntervalMs = 200;

		return std::min(std::max(interval, kMinFeedbackIntervalMs), kMaxFeedbackIntervalMs);
	}

	bool AimdRateControl::TimeToReduceFurther(int64_t timeNow, uint32_t incomingBitrateBps) const
	{
		MS_TRACE();

		const int64_t bitrateReductionInterval = std::max<int64_t>(std::min<int64_t>(this->rtt, 200), 10);

		if (timeNow - this->timeLastBitrateChange >= bitrateReductionInterval)
			return true;

		if (ValidEstimate())
		{
			// TODO(terelius/holmer): Investigate consequences of increasing
			// the threshold to 0.95 * LatestEstimate().
			const uint32_t threshold = static_cast<uint32_t>(0.5 * LatestEstimate());

			return incomingBitrateBps < threshold;
		}

		return false;
	}

	void AimdRateControl::Update(const RateControlInput* input, int64_t nowMs)
	{
		MS_TRACE();

		MS_ASSERT(input, "input missing");

		// Set the initial bit rate value to what we're receiving the first half
		// second.
		if (!this->bitrateIsInitialized)
		{
			const int64_t kInitializationTimeMs = 5000;

			// MS_ASSERT(kBitrateWindowMs <= kInitializationTimeMs);

			if (this->timeFirstIncomingEstimate < 0)
			{
				if (input->incomingBitrate)
					this->timeFirstIncomingEstimate = nowMs;
			}
			else if (nowMs - this->timeFirstIncomingEstimate > kInitializationTimeMs && input->incomingBitrate)
			{
				this->currentBitrateBps = input->incomingBitrate;
				this->bitrateIsInitialized = true;
			}
		}

		if (this->updated && this->currentInput.bwState == kBwOverusing)
		{
			// Only update delay factor and incoming bit rate. We always want to react
			// on an over-use.
			this->currentInput.noiseVar = input->noiseVar;
			this->currentInput.incomingBitrate = input->incomingBitrate;
		}
		else
		{
			this->updated = true;
			this->currentInput = *input;
		}
	}

	int AimdRateControl::GetNearMaxIncreaseRateBps() const
	{
		MS_TRACE();

		// MS_ASSERT(this->currentBitrateBps > 0);

		double bitsPerFrame = static_cast<double>(this->currentBitrateBps) / 30.0;
		double packetsPerFrame = std::ceil(bitsPerFrame / (8.0 * 1200.0));
		double avgPacketSizeBits = bitsPerFrame / packetsPerFrame;
		// Approximate the over-use estimator delay to 100 ms.
		const int64_t responseTime = (this->rtt + 100) * 2;
		constexpr double kMinIncreaseRateBps = 4000;

		return static_cast<int>(std::max(kMinIncreaseRateBps,
			(avgPacketSizeBits * 1000) / responseTime));
	}

	uint32_t AimdRateControl::ChangeBitrate(uint32_t newBitrateBps, uint32_t incomingBitrateBps, int64_t nowMs)
	{
		MS_TRACE();

		if (!this->updated)
			return this->currentBitrateBps;

		// An over-use should always trigger us to reduce the bitrate, even though
		// we have not yet established our first estimate. By acting on the over-use,
		// we will end up with a valid estimate.
		if (!this->bitrateIsInitialized && this->currentInput.bwState != kBwOverusing)
			return this->currentBitrateBps;

		this->updated = false;
		ChangeState(this->currentInput, nowMs);

		// Calculated here because it's used in multiple places.
		const float incomingBitrateKbps = incomingBitrateBps / 1000.0f;
		// Calculate the max bit rate std dev given the normalized
		// variance and the current incoming bit rate.
		const float stdMaxBitRate = sqrt(this->varMaxBitrateKbps * this->avgMaxBitrateKbps);

		switch (this->rateControlState)
		{
			case kRcHold:
				break;

			case kRcIncrease:
				if (this->avgMaxBitrateKbps >= 0 && incomingBitrateKbps > this->avgMaxBitrateKbps + 3 * stdMaxBitRate)
				{
					ChangeRegion(kRcMaxUnknown);
					this->avgMaxBitrateKbps = -1.0;
				}
				if (this->rateControlRegion == kRcNearMax)
				{
					uint32_t additiveIncreaseBps = AdditiveRateIncrease(nowMs, this->timeLastBitrateChange);

					newBitrateBps += additiveIncreaseBps;
				}
				else
				{
					uint32_t multiplicativeIncreaseBps = MultiplicativeRateIncrease(nowMs, this->timeLastBitrateChange, newBitrateBps);

					newBitrateBps += multiplicativeIncreaseBps;
				}

				this->timeLastBitrateChange = nowMs;
				break;

			case kRcDecrease:
				this->bitrateIsInitialized = true;
				// Set bit rate to something slightly lower than max
				// to get rid of any self-induced delay.
				newBitrateBps = static_cast<uint32_t>(this->beta * incomingBitrateBps + 0.5);

				if (newBitrateBps > this->currentBitrateBps)
				{
					// Avoid increasing the rate when over-using.
					if (this->rateControlRegion != kRcMaxUnknown)
					{
						newBitrateBps = static_cast<uint32_t>(this->beta * this->avgMaxBitrateKbps * 1000 + 0.5f);
					}
					newBitrateBps = std::min(newBitrateBps, this->currentBitrateBps);
				}

				ChangeRegion(kRcNearMax);

				if (incomingBitrateBps < this->currentBitrateBps)
				{
					this->lastDecrease = int(this->currentBitrateBps - newBitrateBps);
				}

				if (incomingBitrateKbps < this->avgMaxBitrateKbps - 3 * stdMaxBitRate)
				{
					this->avgMaxBitrateKbps = -1.0f;
				}

				UpdateMaxBitRateEstimate(incomingBitrateKbps);
				// Stay on hold until the pipes are cleared.
				ChangeState(kRcHold);
				this->timeLastBitrateChange = nowMs;
				break;

			default:
				MS_ASSERT(false, "invalid this->rateControlState value");
		}

		return ClampBitrate(newBitrateBps, incomingBitrateBps);
	}

	uint32_t AimdRateControl::ClampBitrate(uint32_t newBitrateBps, uint32_t incomingBitrateBps) const
	{
		MS_TRACE();

		// Don't change the bit rate if the send side is too far off.
		// We allow a bit more lag at very low rates to not too easily get stuck if
		// the encoder produces uneven outputs.
		const uint32_t maxBitrateBps = static_cast<uint32_t>(1.5f * incomingBitrateBps) + 10000;

		if (newBitrateBps > this->currentBitrateBps && newBitrateBps > maxBitrateBps)
		{
			newBitrateBps = std::max(this->currentBitrateBps, maxBitrateBps);
		}
		newBitrateBps = std::max(newBitrateBps, this->minConfiguredBitrateBps);

		return newBitrateBps;
	}

	uint32_t AimdRateControl::MultiplicativeRateIncrease(int64_t nowMs, int64_t lastMs, uint32_t currentBitrateBps) const
	{
		MS_TRACE();

		double alpha = 1.08;

		if (lastMs > -1)
		{
			int timeSinceLastUpdateMs = std::min(static_cast<int>(nowMs - lastMs), 1000);

			alpha = pow(alpha, timeSinceLastUpdateMs / 1000.0);
		}

		uint32_t multiplicativeIncreaseBps = std::max(currentBitrateBps * (alpha - 1.0), 1000.0);

		return multiplicativeIncreaseBps;
	}

	void AimdRateControl::UpdateMaxBitRateEstimate(float incomingBitrateKbps)
	{
		MS_TRACE();

		const float alpha = 0.05f;

		if (this->avgMaxBitrateKbps == -1.0f)
		{
			this->avgMaxBitrateKbps = incomingBitrateKbps;
		}
		else
		{
			this->avgMaxBitrateKbps = (1 - alpha) * this->avgMaxBitrateKbps + alpha * incomingBitrateKbps;
		}

		// Estimate the max bit rate variance and normalize the variance
		// with the average max bit rate.
		const float norm = std::max(this->avgMaxBitrateKbps, 1.0f);

		this->varMaxBitrateKbps = (1 - alpha) * this->varMaxBitrateKbps +
			alpha * (this->avgMaxBitrateKbps - incomingBitrateKbps) * (this->avgMaxBitrateKbps - incomingBitrateKbps) / norm;

		// 0.4 ~= 14 kbit/s at 500 kbit/s
		if (this->varMaxBitrateKbps < 0.4f)
		{
			this->varMaxBitrateKbps = 0.4f;
		}
		// 2.5f ~= 35 kbit/s at 500 kbit/s
		if (this->varMaxBitrateKbps > 2.5f)
		{
			this->varMaxBitrateKbps = 2.5f;
		}
	}

	void AimdRateControl::ChangeState(const RateControlInput& input, int64_t nowMs)
	{
		MS_TRACE();

		(void)input;
		switch (this->currentInput.bwState)
		{
			case kBwNormal:
				if (this->rateControlState == kRcHold)
				{
					this->timeLastBitrateChange = nowMs;
					ChangeState(kRcIncrease);
				}
				break;
			case kBwOverusing:
				if (this->rateControlState != kRcDecrease)
				{
					ChangeState(kRcDecrease);
				}
				break;
			case kBwUnderusing:
				ChangeState(kRcHold);
				break;
			default:
				MS_ASSERT(false, "invalid RateControlInput::bwState value");
		}
	}
}
