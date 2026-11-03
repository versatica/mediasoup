#ifndef MS_FUZZER_RTC_NEW_RTCP_DLRR_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_NEW_RTCP_DLRR_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"

namespace FuzzerRtcRtcpDlrrExtendedReport
{
	void Fuzz(const RTC::NEW_RTCP::DlrrExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpDlrrExtendedReport

#endif
