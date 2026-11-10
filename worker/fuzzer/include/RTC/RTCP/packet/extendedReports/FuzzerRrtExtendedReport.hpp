#ifndef MS_FUZZER_RTC_RTCP_RRT_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_RTCP_RRT_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/extendedReports/RrtExtendedReport.hpp"

namespace FuzzerRtcRtcpRrtExtendedReport
{
	void Fuzz(const RTC::RTCP::RrtExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpRrtExtendedReport

#endif
