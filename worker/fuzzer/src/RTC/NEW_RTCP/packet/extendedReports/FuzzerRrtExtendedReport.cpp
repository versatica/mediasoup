#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerRrtExtendedReport.hpp"

void FuzzerRtcRtcpRrtExtendedReport::Fuzz(const RTC::NEW_RTCP::RrtExtendedReport* extendedReport)
{
	extendedReport->GetNtp();
}
