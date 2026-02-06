#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TRANSPORT
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TRANSPORT

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

namespace FuzzerRtcRtcpFeedbackRtpTransport
{
	void Fuzz(RTC::RTCP::FeedbackRtpTransportPacket* packet);
}

#endif
