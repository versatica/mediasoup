#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIPv4AddressParameter.hpp"

void FuzzerRtcSctpIPv4AddressParameter::Fuzz(const RTC::SCTP::IPv4AddressParameter* parameter)
{
	parameter->GetIPv4Address();
}
