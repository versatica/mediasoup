#ifndef MS_FUZZER_RTC_RTCP_RECEIVER_REPORT
#define MS_FUZZER_RTC_RTCP_RECEIVER_REPORT

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace ReceiverReport
			{
				void Fuzz(::RTC::RTCP::ReceiverReportPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
