#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerAddIncomingStreamsRequestParameter.hpp"

void FuzzerRtcSctpAddIncomingStreamsRequestParameter::Fuzz(
  const RTC::SCTP::AddIncomingStreamsRequestParameter* parameter)
{
	parameter->GetReconfigurationRequestSequenceNumber();
	parameter->GetNumberOfNewStreams();
}
