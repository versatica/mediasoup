#ifndef MS_RTC_REMB_RATE_CONTROL_REGION_HPP
#define MS_RTC_REMB_RATE_CONTROL_REGION_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	namespace REMB
	{
		enum RateControlRegion
		{
			RC_NEAR_MAX,
			RC_ABOVE_MAX,
			RC_MAX_UNKNOWN
		};
	} // namespace REMB
} // namespace RTC

#endif
