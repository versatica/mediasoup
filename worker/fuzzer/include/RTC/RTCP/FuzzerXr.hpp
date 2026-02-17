#ifndef MS_FUZZER_RTC_RTCP_XR_PACKET
#define MS_FUZZER_RTC_RTCP_XR_PACKET

#include "common.hpp"
#include "RTC/RTCP/XR.hpp"

namespace FuzzerRtcRtcpExtendedReport
{
	void Fuzz(RTC::RTCP::ExtendedReportPacket* packet);
}

#endif
