#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_TST
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_TST

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsTstn
			{
				void Fuzz(::RTC::RTCP::FeedbackPsTstnPacket* packet);
			}

			namespace FeedbackPsTstr
			{
				void Fuzz(::RTC::RTCP::FeedbackPsTstrPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
