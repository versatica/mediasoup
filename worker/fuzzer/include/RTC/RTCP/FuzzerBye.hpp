#ifndef MS_FUZZER_RTC_RTCP_BYE
#define MS_FUZZER_RTC_RTCP_BYE

#include "common.hpp"
#include "RTC/RTCP/Bye.hpp"

namespace FuzzerRtcRtcpBye
{
	void Fuzz(RTC::RTCP::ByePacket* packet);
}

#endif
