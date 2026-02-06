#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"

namespace FuzzerRtcRtcpFeedbackPs
{
	void Fuzz(RTC::RTCP::Packet* packet);
}

#endif
