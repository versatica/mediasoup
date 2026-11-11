#ifndef MS_FUZZER_RTC_SCTP_SUPPORTED_EXTENSIONS_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_SUPPORTED_EXTENSIONS_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"

namespace FuzzerRtcSctpSupportedExtensionsParameter
{
	void Fuzz(const RTC::SCTP::SupportedExtensionsParameter* parameter);
} // namespace FuzzerRtcSctpSupportedExtensionsParameter

#endif
