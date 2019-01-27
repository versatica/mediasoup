#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_RPSI
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_RPSI

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRpsi.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsRpsi
			{
				void Fuzz(::RTC::RTCP::FeedbackPsRpsiPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
