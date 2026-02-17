#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_TST
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_TST

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"

namespace FuzzerRtcRtcpFeedbackPsTstn
{
	void Fuzz(RTC::RTCP::FeedbackPsTstnPacket* packet);
}

namespace FuzzerRtcRtcpFeedbackPsTstr
{
	void Fuzz(RTC::RTCP::FeedbackPsTstrPacket* packet);
}

#endif
