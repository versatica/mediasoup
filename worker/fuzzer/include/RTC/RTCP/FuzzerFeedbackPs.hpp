#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_PS
#define MS_FUZZER_RTC_RTCP_FEEDBACK_PS

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackPs
			{
				void Fuzz(::RTC::RTCP::Packet* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
