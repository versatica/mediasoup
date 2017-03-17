#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_INPUT_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_INPUT_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/BandwidthUsage.hpp"

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	struct RateControlInput
	{
		RateControlInput(BandwidthUsage bw_state, const uint32_t incoming_bitrate, double noise_var);

		BandwidthUsage bw_state;
		uint32_t incoming_bitrate;
		double noise_var;
	};

	inline
	RateControlInput::RateControlInput(BandwidthUsage bw_state, const uint32_t incoming_bitrate, double noise_var) :
		bw_state(bw_state),
		incoming_bitrate(incoming_bitrate),
		noise_var(noise_var)
	{}
}

#endif
