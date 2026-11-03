#ifndef MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_EXTENDED_REPORT_HPP
#define MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/UnknownExtendedReport.hpp"

namespace FuzzerRtcRtcpUnknownExtendedReport
{
	void Fuzz(const RTC::NEW_RTCP::UnknownExtendedReport* extendedReport);
} // namespace FuzzerRtcRtcpUnknownExtendedReport

#endif
