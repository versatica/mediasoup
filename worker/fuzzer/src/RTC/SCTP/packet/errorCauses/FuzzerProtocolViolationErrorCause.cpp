#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerProtocolViolationErrorCause.hpp"

void FuzzerRtcSctpProtocolViolationErrorCause::Fuzz(
  const RTC::SCTP::ProtocolViolationErrorCause* errorCause)
{
	errorCause->HasAdditionalInformation();
	errorCause->GetAdditionalInformation();
	errorCause->GetAdditionalInformationLength();
}
