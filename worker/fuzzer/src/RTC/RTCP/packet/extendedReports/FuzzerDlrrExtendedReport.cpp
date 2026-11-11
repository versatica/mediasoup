#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerDlrrExtendedReport.hpp"

void FuzzerRtcRtcpDlrrExtendedReport::Fuzz(const RTC::RTCP::DlrrExtendedReport* extendedReport)
{
	extendedReport->GetNumberOfSubBlocks();
	extendedReport->GetSubBlocks();
}
