#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_ECN
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_ECN

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"

namespace FuzzerRtcRtcpFeedbackRtpEcn
{
	void Fuzz(RTC::RTCP::FeedbackRtpEcnPacket* packet);
}

#endif
