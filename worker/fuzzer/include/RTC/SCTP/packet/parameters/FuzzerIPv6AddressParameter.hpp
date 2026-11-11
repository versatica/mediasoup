#ifndef MS_FUZZER_RTC_SCTP_IPV6_ADDRESS_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_IPV6_ADDRESS_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"

namespace FuzzerRtcSctpIPv6AddressParameter
{
	void Fuzz(const RTC::SCTP::IPv6AddressParameter* parameter);
} // namespace FuzzerRtcSctpIPv6AddressParameter

#endif
