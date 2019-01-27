#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtp
			{
				void Fuzz(::RTC::RTCP::Packet* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
