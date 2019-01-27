#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_ECN
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_ECN

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpEcn
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpEcnPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
