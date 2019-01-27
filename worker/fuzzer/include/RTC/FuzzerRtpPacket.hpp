#ifndef MS_FUZZER_RTC_RTP_PACKET_HPP
#define MS_FUZZER_RTC_RTP_PACKET_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RtpPacket
		{
			void Fuzz(const uint8_t* data, size_t len);
		}
	} // namespace RTC
} // namespace Fuzzer

#endif
