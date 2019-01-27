#ifndef MS_FUZZER_RTC_RTCP_SDES
#define MS_FUZZER_RTC_RTCP_SDES

#include "common.hpp"
#include "RTC/RTCP/Sdes.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace Sdes
			{
				void Fuzz(::RTC::RTCP::SdesPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
