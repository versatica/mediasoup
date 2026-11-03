#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerDlrrExtendedReport.hpp"

void FuzzerRtcRtcpDlrrExtendedReport::Fuzz(const RTC::NEW_RTCP::DlrrExtendedReport* extendedReport)
{
	extendedReport->GetNumberOfSubBlocks();
	extendedReport->GetSubBlocks();
}
