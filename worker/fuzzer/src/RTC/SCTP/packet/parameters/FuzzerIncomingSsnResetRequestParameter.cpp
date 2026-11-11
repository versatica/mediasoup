#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIncomingSsnResetRequestParameter.hpp"

void FuzzerRtcSctpIncomingSsnResetRequestParameter::Fuzz(
  const RTC::SCTP::IncomingSsnResetRequestParameter* parameter)
{
	parameter->GetReconfigurationRequestSequenceNumber();
	parameter->GetStreamIds();
}
