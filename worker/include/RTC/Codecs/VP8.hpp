#ifndef MS_RTC_CODECS_VP8_HPP
#define MS_RTC_CODECS_VP8_HPP

#include "common.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		class VP8
		{
		public:
			static void ProcessRtpPacket(RTC::RtpPacket* packet);
		};
	} // namespace Codecs
} // namespace RTC

#endif
