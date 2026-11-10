#ifndef MS_FUZZER_RTC_RTCP_UNKNOWN_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_RTCP_UNKNOWN_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/extendedReports/UnknownExtendedReport.hpp"

namespace FuzzerRtcRtcpUnknownExtendedReport
{
	void Fuzz(const RTC::RTCP::UnknownExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpUnknownExtendedReport

#endif
