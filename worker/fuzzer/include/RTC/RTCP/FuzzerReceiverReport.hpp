#ifndef MS_FUZZER_RTC_RTCP_RECEIVER_REPORT
#define MS_FUZZER_RTC_RTCP_RECEIVER_REPORT

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"

namespace FuzzerRtcRtcpReceiverReport
{
	void Fuzz(RTC::RTCP::ReceiverReportPacket* packet);
}

#endif
