#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerNoUserDataErrorCause.hpp"

void FuzzerRtcSctpNoUserDataErrorCause::Fuzz(const RTC::SCTP::NoUserDataErrorCause* errorCause)
{
	errorCause->GetTsn();
}
