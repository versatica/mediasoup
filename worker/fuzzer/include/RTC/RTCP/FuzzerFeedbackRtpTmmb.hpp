#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TMMB
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TMMB

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpTmmbn
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpTmmbnPacket* packet);
			}

			namespace FeedbackRtpTmmbr
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpTmmbrPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
