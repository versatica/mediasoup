#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_SR_REQ
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_SR_REQ

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpSrReq.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpSrReq
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpSrReqPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
