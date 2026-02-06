#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_FIR
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_FIR

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"

namespace FuzzerRtcRtcpFeedbackPsFir
{
	void Fuzz(RTC::RTCP::FeedbackPsFirPacket* packet);
}

#endif
