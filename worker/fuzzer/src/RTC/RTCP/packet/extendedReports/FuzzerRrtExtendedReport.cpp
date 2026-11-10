#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerRrtExtendedReport.hpp"

void FuzzerRtcRtcpRrtExtendedReport::Fuzz(const RTC::RTCP::RrtExtendedReport* extendedReport)
{
	extendedReport->GetNtp();
}
