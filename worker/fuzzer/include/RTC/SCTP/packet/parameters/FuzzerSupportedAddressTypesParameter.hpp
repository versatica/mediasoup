#ifndef MS_FUZZER_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/SupportedAddressTypesParameter.hpp"

namespace FuzzerRtcSctpSupportedAddressTypesParameter
{
	void Fuzz(const RTC::SCTP::SupportedAddressTypesParameter* parameter);
} // namespace FuzzerRtcSctpSupportedAddressTypesParameter

#endif
