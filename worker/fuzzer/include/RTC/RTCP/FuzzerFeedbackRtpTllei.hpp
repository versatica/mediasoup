#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TLLEI
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TLLEI

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTllei.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpTllei
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpTlleiPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
