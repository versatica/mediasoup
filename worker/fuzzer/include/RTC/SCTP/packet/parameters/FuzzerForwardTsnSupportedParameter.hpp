#ifndef MS_FUZZER_RTC_SCTP_FORWARD_TSN_SUPPORTED_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_FORWARD_TSN_SUPPORTED_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"

namespace FuzzerRtcSctpForwardTsnSupportedParameter
{
	void Fuzz(const RTC::SCTP::ForwardTsnSupportedParameter* parameter);
} // namespace FuzzerRtcSctpForwardTsnSupportedParameter

#endif
