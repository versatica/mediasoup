#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_AFB
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_AFB

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsAfb
			{
				void Fuzz(::RTC::RTCP::FeedbackPsAfbPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
