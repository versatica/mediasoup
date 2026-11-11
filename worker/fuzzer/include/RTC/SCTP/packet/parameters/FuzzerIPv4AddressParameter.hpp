#ifndef MS_FUZZER_RTC_SCTP_IPV4_ADDRESS_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_IPV4_ADDRESS_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"

namespace FuzzerRtcSctpIPv4AddressParameter
{
	void Fuzz(const RTC::SCTP::IPv4AddressParameter* parameter);
} // namespace FuzzerRtcSctpIPv4AddressParameter

#endif
