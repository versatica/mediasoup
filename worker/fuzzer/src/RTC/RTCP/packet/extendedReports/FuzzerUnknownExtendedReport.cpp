#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerUnknownExtendedReport.hpp"

void FuzzerRtcRtcpUnknownExtendedReport::Fuzz(const RTC::RTCP::UnknownExtendedReport* extendedReport)
{
	extendedReport->HasUnknownValue();
	extendedReport->GetUnknownValue();
	extendedReport->GetUnknownValueLength();
}
