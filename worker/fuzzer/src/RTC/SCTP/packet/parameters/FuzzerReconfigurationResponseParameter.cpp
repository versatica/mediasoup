#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerReconfigurationResponseParameter.hpp"

void FuzzerRtcSctpReconfigurationResponseParameter::Fuzz(
  const RTC::SCTP::ReconfigurationResponseParameter* parameter)
{
	parameter->GetReconfigurationResponseSequenceNumber();
	parameter->GetResult();
	parameter->HasUnknownResult();
	parameter->HasNextTsns();
	parameter->GetSenderNextTsn();
	parameter->GetReceiverNextTsn();
}
