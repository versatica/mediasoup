#ifndef MS_FUZZER_RTC_RTCP_DLRR_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_RTCP_DLRR_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/extendedReports/DlrrExtendedReport.hpp"

namespace FuzzerRtcRtcpDlrrExtendedReport
{
	void Fuzz(const RTC::RTCP::DlrrExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpDlrrExtendedReport

#endif
