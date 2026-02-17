#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_REMB
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_REMB

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"

namespace FuzzerRtcRtcpFeedbackPsRemb
{
	void Fuzz(RTC::RTCP::FeedbackPsRembPacket* packet);
}

#endif
