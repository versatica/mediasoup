#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnrecognizedParametersErrorCause.hpp"

void FuzzerRtcSctpUnrecognizedParametersErrorCause::Fuzz(
  const RTC::SCTP::UnrecognizedParametersErrorCause* errorCause)
{
	errorCause->HasUnrecognizedParameters();
	errorCause->GetUnrecognizedParameters();
	errorCause->GetUnrecognizedParametersLength();
}
