#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_SLI
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_SLI

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsSli.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsSli
			{
				void Fuzz(::RTC::RTCP::FeedbackPsSliPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
