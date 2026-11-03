#ifndef MS_FUZZER_RTC_NEW_RTCP_RRT_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_NEW_RTCP_RRT_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/RrtExtendedReport.hpp"

namespace FuzzerRtcRtcpRrtExtendedReport
{
	void Fuzz(const RTC::NEW_RTCP::RrtExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpRrtExtendedReport

#endif
