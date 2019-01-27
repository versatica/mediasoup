#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_NACK
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_NACK

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpNack
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpNackPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
