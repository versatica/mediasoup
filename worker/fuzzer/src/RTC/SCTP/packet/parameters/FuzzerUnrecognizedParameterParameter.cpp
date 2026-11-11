#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerUnrecognizedParameterParameter.hpp"

void FuzzerRtcSctpUnrecognizedParameterParameter::Fuzz(
  const RTC::SCTP::UnrecognizedParameterParameter* parameter)
{
	parameter->HasUnrecognizedParameter();
	parameter->GetUnrecognizedParameter();
	parameter->GetUnrecognizedParameterLength();
}
