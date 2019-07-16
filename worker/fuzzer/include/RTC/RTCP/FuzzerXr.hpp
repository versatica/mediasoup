#ifndef MS_FUZZER_RTC_RTCP_XR_PACKET
#define MS_FUZZER_RTC_RTCP_XR_PACKET

#include "common.hpp"
#include "RTC/RTCP/XR.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace ExtendedReport
			{
				void Fuzz(::RTC::RTCP::ExtendedReportPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
