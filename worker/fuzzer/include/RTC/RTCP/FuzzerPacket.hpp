#ifndef MS_FUZZER_RTC_RTCP_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_PACKET_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace Packet
			{
				void Fuzz(const uint8_t* data, size_t len);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
