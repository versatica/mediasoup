#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnknownErrorCause.hpp"

void FuzzerRtcSctpUnknownErrorCause::Fuzz(const RTC::SCTP::UnknownErrorCause* errorCause)
{
	errorCause->HasUnknownValue();
	errorCause->GetUnknownValue();
	errorCause->GetUnknownValueLength();
}
