#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerOutgoingSsnResetRequestParameter.hpp"

void FuzzerRtcSctpOutgoingSsnResetRequestParameter::Fuzz(
  const RTC::SCTP::OutgoingSsnResetRequestParameter* parameter)
{
	parameter->GetReconfigurationRequestSequenceNumber();
	parameter->GetReconfigurationResponseSequenceNumber();
	parameter->GetSenderLastAssignedTsn();
	parameter->GetStreamIds();
}
