#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerStaleCookieErrorCause.hpp"

void FuzzerRtcSctpStaleCookieErrorCause::Fuzz(const RTC::SCTP::StaleCookieErrorCause* errorCause)
{
	errorCause->GetMeasureOfStaleness();
}
