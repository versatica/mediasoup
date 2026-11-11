#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerMissingMandatoryParameterErrorCause.hpp"

void FuzzerRtcSctpMissingMandatoryParameterErrorCause::Fuzz(
  const RTC::SCTP::MissingMandatoryParameterErrorCause* errorCause)
{
	errorCause->GetNumberOfMissingParameters();

	for (uint32_t idx{ 0 }; idx < errorCause->GetNumberOfMissingParameters(); ++idx)
	{
		errorCause->GetMissingParameterTypeAt(idx);
	}
}
