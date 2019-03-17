#ifndef MS_RTC_REMB_BANDWIDTH_USAGE_HPP
#define MS_RTC_REMB_BANDWIDTH_USAGE_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	namespace REMB
	{
		enum BandwidthUsage
		{
			BW_NORMAL,
			BW_UNDERUSING,
			BW_OVERUSING
		};
	} // namespace REMB
} // namespace RTC

#endif
