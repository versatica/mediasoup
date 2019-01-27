#ifndef MS_FUZZER_RTC_RTCP_BYE
#define MS_FUZZER_RTC_RTCP_BYE

#include "common.hpp"
#include "RTC/RTCP/Bye.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace Bye
			{
				void Fuzz(::RTC::RTCP::ByePacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
