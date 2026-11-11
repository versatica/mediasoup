#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerUnknownParameter.hpp"

void FuzzerRtcSctpUnknownParameter::Fuzz(const RTC::SCTP::UnknownParameter* parameter)
{
	parameter->HasUnknownValue();
	parameter->GetUnknownValue();
	parameter->GetUnknownValueLength();
}
