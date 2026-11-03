#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerUnknownExtendedReport.hpp"

void FuzzerRtcRtcpUnknownExtendedReport::Fuzz(const RTC::NEW_RTCP::UnknownExtendedReport* extendedReport)
{
	extendedReport->HasUnknownValue();
	extendedReport->GetUnknownValue();
	extendedReport->GetUnknownValueLength();
}
