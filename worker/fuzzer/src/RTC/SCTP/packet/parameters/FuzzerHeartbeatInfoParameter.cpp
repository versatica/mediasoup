#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerHeartbeatInfoParameter.hpp"

void FuzzerRtcSctpHeartbeatInfoParameter::Fuzz(const RTC::SCTP::HeartbeatInfoParameter* parameter)
{
	parameter->HasInfo();
	parameter->GetInfo();
	parameter->GetInfoLength();
}
