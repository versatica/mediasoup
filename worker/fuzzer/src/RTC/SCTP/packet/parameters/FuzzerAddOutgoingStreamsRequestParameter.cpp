#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerAddOutgoingStreamsRequestParameter.hpp"

void FuzzerRtcSctpAddOutgoingStreamsRequestParameter::Fuzz(
  const RTC::SCTP::AddOutgoingStreamsRequestParameter* parameter)
{
	parameter->GetReconfigurationRequestSequenceNumber();
	parameter->GetNumberOfNewStreams();
}
