#ifndef MS_FUZZER_RTC_RTCP_SDES
#define MS_FUZZER_RTC_RTCP_SDES

#include "common.hpp"
#include "RTC/RTCP/Sdes.hpp"

namespace FuzzerRtcRtcpSdes
{
	void Fuzz(RTC::RTCP::SdesPacket* packet);
}

#endif
