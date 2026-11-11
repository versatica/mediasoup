#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerZeroChecksumAcceptableParameter.hpp"

void FuzzerRtcSctpZeroChecksumAcceptableParameter::Fuzz(
  const RTC::SCTP::ZeroChecksumAcceptableParameter* parameter)
{
	parameter->GetAlternateErrorDetectionMethod();
}
