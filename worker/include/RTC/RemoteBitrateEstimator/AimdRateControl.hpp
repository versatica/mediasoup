/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/RateControlRegion.hpp"
#include "RTC/RemoteBitrateEstimator/RateControlInput.hpp"
#include "RTC/RemoteBitrateEstimator/OveruseDetector.hpp"

// A rate control implementation based on additive increases of
// bitrate when no over-use is detected and multiplicative decreases when
// over-uses are detected. When we think the available bandwidth has changes or
// is unknown, we will switch to a "slow-start mode" where we increase
// multiplicatively.

namespace RTC
{
	class AimdRateControl
	{
	private:
		enum RateControlState
		{
			kRcHold,
			kRcIncrease,
			kRcDecrease
		};

	public:
		AimdRateControl();
		virtual ~AimdRateControl();

		// Returns true if there is a valid estimate of the incoming bitrate, false
		// otherwise.
		bool ValidEstimate() const;
		void SetStartBitrate(int start_bitrate_bps);
		void SetMinBitrate(int min_bitrate_bps);
		int64_t GetFeedbackInterval() const;
		// Returns true if the bitrate estimate hasn't been changed for more than
		// an RTT, or if the incoming_bitrate is less than half of the current
		// estimate. Should be used to decide if we should reduce the rate further
		// when over-using.
		bool TimeToReduceFurther(int64_t time_now, uint32_t incoming_bitrate_bps) const;
		uint32_t LatestEstimate() const;
		uint32_t UpdateBandwidthEstimate(int64_t now_ms);
		void SetRtt(int64_t rtt);
		void Update(const RateControlInput* input, int64_t now_ms);
		void SetEstimate(int bitrate_bps, int64_t now_ms);

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
		uint32_t ChangeBitrate(uint32_t current_bitrate, uint32_t incoming_bitrate, int64_t now_ms);
		// Clamps new_bitrate_bps to within the configured min bitrate and a linear
		// function of the incoming bitrate, so that the new bitrate can't grow too
		// large compared to the bitrate actually being received by the other end.
		uint32_t ClampBitrate(uint32_t new_bitrate_bps, uint32_t incoming_bitrate_bps) const;
		uint32_t MultiplicativeRateIncrease(int64_t now_ms, int64_t last_ms, uint32_t current_bitrate_bps) const;
		uint32_t AdditiveRateIncrease(int64_t now_ms, int64_t last_ms) const;
		void UpdateChangePeriod(int64_t now_ms);
		void UpdateMaxBitRateEstimate(float incoming_bit_rate_kbps);
		void ChangeState(const RateControlInput& input, int64_t now_ms);
		void ChangeState(RateControlState new_state);
		void ChangeRegion(RateControlRegion region);

	private:
		static constexpr int64_t kDefaultRttMs = 200;

		// (jmillan) replacement from 'congestion_controller::GetMinBitrateBps()'.
		static constexpr int kMinBitrateBps = 10000;

		uint32_t min_configured_bitrate_bps_ = kMinBitrateBps;
		uint32_t max_configured_bitrate_bps_ = 30000000;
		uint32_t current_bitrate_bps_ = max_configured_bitrate_bps_;
		float avg_max_bitrate_kbps_ = -1.0f;
		float var_max_bitrate_kbps_ = 0.4f;
		RateControlState rate_control_state_ = kRcHold;
		RateControlRegion rate_control_region_ = kRcMaxUnknown;
		int64_t time_last_bitrate_change_ = -1;
		RateControlInput current_input_;
		bool updated_ = false;
		int64_t time_first_incoming_estimate_ = -1;
		bool bitrate_is_initialized_ = false;
		float beta_ = 0.85f;
		int64_t rtt_ = kDefaultRttMs;
		bool in_experiment_;
		int last_decrease_;
	};

	inline
	AimdRateControl::AimdRateControl() :
		current_input_(kBwNormal, 0, 1.0)
	{
		// TODO: (jmillan) do it configurable
		//in_experiment_ = false;
		in_experiment_ = !AdaptiveThresholdExperimentIsDisabled();
	}

	inline
	AimdRateControl::~AimdRateControl()
	{}

}  // namespace RTC

#endif  // MS_RTC_REMOTE_BITRATE_ESTIMATOR_AIMD_RATE_CONTROL_HPP
