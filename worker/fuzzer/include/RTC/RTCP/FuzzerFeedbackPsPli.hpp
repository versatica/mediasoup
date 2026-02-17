#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_PLI
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_PLI

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"

namespace FuzzerRtcRtcpFeedbackPsPli
{
	void Fuzz(RTC::RTCP::FeedbackPsPliPacket* packet);
}

#endif
