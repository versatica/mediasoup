#ifndef MS_FUZZER_RTC_RTCP_SENDER_REPORT
#define MS_FUZZER_RTC_RTCP_SENDER_REPORT

#include "common.hpp"
#include "RTC/RTCP/SenderReport.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace SenderReport
			{
				void Fuzz(::RTC::RTCP::SenderReportPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
