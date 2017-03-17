#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_BANDWIDTH_USAGE_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_BANDWIDTH_USAGE_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	enum BandwidthUsage
	{
		kBwNormal,
		kBwUnderusing,
		kBwOverusing
	};
}

#endif
