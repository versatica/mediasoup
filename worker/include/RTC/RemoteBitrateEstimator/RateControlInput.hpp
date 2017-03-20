#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_INPUT_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_INPUT_HPP

#include "common.hpp"
#include "RTC/RemoteBitrateEstimator/BandwidthUsage.hpp"

// webrtc/modules/remote_bitrate_estimator/include/bweDefines.h

namespace RTC
{
	struct RateControlInput
	{
		RateControlInput(BandwidthUsage bwState, const uint32_t incomingBitrate, double noiseVar);

		BandwidthUsage bwState;
		uint32_t incomingBitrate;
		double noiseVar;
	};

	/* Inline methods. */

	inline
	RateControlInput::RateControlInput(BandwidthUsage bwState, const uint32_t incomingBitrate, double noiseVar) :
		bwState(bwState),
		incomingBitrate(incomingBitrate),
		noiseVar(noiseVar)
	{}
}

#endif
