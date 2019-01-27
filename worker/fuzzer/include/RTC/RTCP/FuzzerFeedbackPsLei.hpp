#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_LEI
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_LEI

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsLei.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsLei
			{
				void Fuzz(::RTC::RTCP::FeedbackPsLeiPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
