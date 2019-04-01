/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMB_SERVER_AIMD_RATE_CONTROL_HPP
#define MS_RTC_REMB_SERVER_AIMD_RATE_CONTROL_HPP

#include "common.hpp"
#include "RTC/RembServer/OveruseDetector.hpp"
#include "RTC/RembServer/RateControlInput.hpp"
#include "RTC/RembServer/RateControlRegion.hpp"

// A rate control implementation based on additive increases of
// bitrate when no over-use is detected and multiplicative decreases when
// over-uses are detected. When we think the available bandwidth has changes or
// is unknown, we will switch to a "slow-start mode" where we increase
// multiplicatively.

namespace RTC
{
	namespace RembServer
	{
		class AimdRateControl
		{
		private:
			static constexpr int64_t DefaultRttMs{ 200 };
			// (jmillan) replacement from 'congestion_controller::GetMinBitrateBps()'.
			static constexpr int MinBitrateBps{ 10000 };

		private:
			enum RateControlState
			{
				RC_HOLD,
				RC_INCREASE,
				RC_DECREASE
			};

		public:
			AimdRateControl();
			virtual ~AimdRateControl() = default;

			// Returns true if there is a valid estimate of the incoming bitrate, false
			// otherwise.
			bool ValidEstimate() const;
			void SetStartBitrate(int startBitrateBps);
			void SetMinBitrate(int minBitrateBps);
			int64_t GetFeedbackInterval() const;
			// Returns true if the bitrate estimate hasn't been changed for more than
			// an RTT, or if the incomingBitrate is less than half of the current
			// estimate. Should be used to decide if we should reduce the rate further
			// when over-using.
			bool TimeToReduceFurther(int64_t timeNow, uint32_t incomingBitrateBps) const;
			uint32_t LatestEstimate() const;
			uint32_t UpdateBandwidthEstimate(int64_t nowMs);
			void SetRtt(int64_t rtt);
			void Update(const RateControlInput* input, int64_t nowMs);
			void SetEstimate(int bitrateBps, int64_t nowMs);

		public:
			// Returns the increase rate which is used when used bandwidth is near the
			// maximal available bandwidth.
			virtual int GetNearMaxIncreaseRateBps() const;
			virtual int GetLastBitrateDecreaseBps() const;

		private:
			// Update the target bitrate according based on, among other things,
			// the current rate control state, the current target bitrate and the incoming
			// bitrate. When in the "increase" state the bitrate will be increased either
			// additively or multiplicatively depending on the rate control region. When
			// in the "decrease" state the bitrate will be decreased to slightly below the
			// incoming bitrate. When in the "hold" state the bitrate will be kept
			// constant to allow built up queues to drain.
			uint32_t ChangeBitrate(uint32_t newBitrateBps, uint32_t incomingBitrateBps, int64_t nowMs);
			// Clamps newBitrateBps to within the configured min bitrate and a linear
			// function of the incoming bitrate, so that the new bitrate can't grow too
			// large compared to the bitrate actually being received by the other end.
			uint32_t ClampBitrate(uint32_t newBitrateBps, uint32_t incomingBitrateBps) const;
			uint32_t MultiplicativeRateIncrease(int64_t nowMs, int64_t lastMs, uint32_t currentBitrateBps) const;
			uint32_t AdditiveRateIncrease(int64_t nowMs, int64_t lastMs) const;
			void UpdateChangePeriod(int64_t nowMs);
			void UpdateMaxBitRateEstimate(float incomingBitrateKbps);
			void ChangeState(const RateControlInput& input, int64_t nowMs);
			void ChangeState(RateControlState newState);
			void ChangeRegion(RateControlRegion region);

		private:
			uint32_t minConfiguredBitrateBps{ MinBitrateBps };
			uint32_t maxConfiguredBitrateBps{ 30000000 };
			uint32_t currentBitrateBps{ this->maxConfiguredBitrateBps };
			float avgMaxBitrateKbps{ -1.0f };
			float varMaxBitrateKbps{ 0.4f };
			RateControlState rateControlState{ RC_HOLD };
			RateControlRegion rateControlRegion{ RC_MAX_UNKNOWN };
			int64_t timeLastBitrateChange{ -1 };
			RateControlInput currentInput;
			bool updated{ false };
			int64_t timeFirstIncomingEstimate{ -1 };
			bool bitrateIsInitialized{ false };
			float beta{ 0.85f };
			int64_t rtt{ DefaultRttMs };
			int lastDecrease{ 0 };
		};

		/* Inline methods. */

		inline AimdRateControl::AimdRateControl() : currentInput(BW_NORMAL, 0, 1.0)
		{
		}

		inline void AimdRateControl::SetStartBitrate(int startBitrateBps)
		{
			this->currentBitrateBps    = startBitrateBps;
			this->bitrateIsInitialized = true;
		}

		inline void AimdRateControl::SetMinBitrate(int minBitrateBps)
		{
			this->minConfiguredBitrateBps = minBitrateBps;
			this->currentBitrateBps       = std::max<int>(minBitrateBps, this->currentBitrateBps);
		}

		inline bool AimdRateControl::ValidEstimate() const
		{
			return this->bitrateIsInitialized;
		}

		inline uint32_t AimdRateControl::LatestEstimate() const
		{
			return this->currentBitrateBps;
		}

		inline uint32_t AimdRateControl::UpdateBandwidthEstimate(int64_t nowMs)
		{
			this->currentBitrateBps =
			  ChangeBitrate(this->currentBitrateBps, this->currentInput.incomingBitrate, nowMs);

			return this->currentBitrateBps;
		}

		inline void AimdRateControl::SetRtt(int64_t rtt)
		{
			this->rtt = rtt;
		}

		inline void AimdRateControl::SetEstimate(int bitrateBps, int64_t nowMs)
		{
			this->updated               = true;
			this->bitrateIsInitialized  = true;
			this->currentBitrateBps     = ClampBitrate(bitrateBps, bitrateBps);
			this->timeLastBitrateChange = nowMs;
		}

		inline int AimdRateControl::GetLastBitrateDecreaseBps() const
		{
			return this->lastDecrease;
		}

		inline uint32_t AimdRateControl::AdditiveRateIncrease(int64_t nowMs, int64_t lastMs) const
		{
			return static_cast<uint32_t>((nowMs - lastMs) * GetNearMaxIncreaseRateBps() / 1000);
		}

		inline void AimdRateControl::ChangeRegion(RateControlRegion region)
		{
			this->rateControlRegion = region;
		}

		inline void AimdRateControl::ChangeState(RateControlState newState)
		{
			this->rateControlState = newState;
		}
	} // namespace RembServer
} // namespace RTC

#endif
