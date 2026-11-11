#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerInvalidStreamIdentifierErrorCause.hpp"

void FuzzerRtcSctpInvalidStreamIdentifierErrorCause::Fuzz(
  const RTC::SCTP::InvalidStreamIdentifierErrorCause* errorCause)
{
	errorCause->GetStreamIdentifier();
}
