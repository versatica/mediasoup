#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSsnTsnResetRequestParameter.hpp"

void FuzzerRtcSctpSsnTsnResetRequestParameter::Fuzz(const RTC::SCTP::SsnTsnResetRequestParameter* parameter)
{
	parameter->GetReconfigurationRequestSequenceNumber();
}
