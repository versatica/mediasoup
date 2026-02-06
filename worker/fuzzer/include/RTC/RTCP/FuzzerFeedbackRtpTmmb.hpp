#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TMMB
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TMMB

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"

namespace FuzzerRtcRtcpFeedbackRtpTmmbn
{
	void Fuzz(RTC::RTCP::FeedbackRtpTmmbnPacket* packet);
}

namespace FuzzerRtcRtcpFeedbackRtpTmmbr
{
	void Fuzz(RTC::RTCP::FeedbackRtpTmmbrPacket* packet);
}

#endif
