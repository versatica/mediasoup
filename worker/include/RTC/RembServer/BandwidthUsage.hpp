#ifndef MS_RTC_REMB_SERVER_BANDWIDTH_USAGE_HPP
#define MS_RTC_REMB_SERVER_BANDWIDTH_USAGE_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	namespace RembServer
	{
		enum BandwidthUsage
		{
			BW_NORMAL,
			BW_UNDERUSING,
			BW_OVERUSING
		};
	} // namespace RembServer
} // namespace RTC

#endif
