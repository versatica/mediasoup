#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_REGION_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_RATE_CONTROL_REGION_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	enum RateControlRegion
	{
		kRcNearMax,
		kRcAboveMax,
		kRcMaxUnknown
	};
}

#endif
