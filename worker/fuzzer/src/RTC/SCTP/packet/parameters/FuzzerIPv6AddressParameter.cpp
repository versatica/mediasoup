#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIPv6AddressParameter.hpp"

void FuzzerRtcSctpIPv6AddressParameter::Fuzz(const RTC::SCTP::IPv6AddressParameter* parameter)
{
	parameter->GetIPv6Address();
}
