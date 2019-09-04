#ifndef MS_RTC_LIBWEBRTC_BANDWIDTH_USAGE_HPP
#define MS_RTC_LIBWEBRTC_BANDWIDTH_USAGE_HPP

// webrtc/modules/remote_bitrate_estimator/include/bwe_defines.h

namespace RTC
{
	namespace libwebrtc
	{
		enum BandwidthUsage
		{
			BW_NORMAL,
			BW_UNDERUSING,
			BW_OVERUSING
		};
	} // namespace libwebrtc
} // namespace RTC

#endif
