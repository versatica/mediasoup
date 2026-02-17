#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

namespace FuzzerRtcRtcpFeedbackRtp
{
	void Fuzz(RTC::RTCP::Packet* packet);
}

#endif
