#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS_VBCM
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS_VBCM

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsVbcm.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPsVbcm
			{
				void Fuzz(::RTC::RTCP::FeedbackPsVbcmPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
