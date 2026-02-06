#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_AFB
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_AFB

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"

namespace FuzzerRtcRtcpFeedbackPsAfb
{
	void Fuzz(RTC::RTCP::FeedbackPsAfbPacket* packet);
}

#endif
