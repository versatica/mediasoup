#ifndef MS_FUZZER_RTC_SCTP_UNRECOGNIZED_PARAMETER_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_UNRECOGNIZED_PARAMETER_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"

namespace FuzzerRtcSctpUnrecognizedParameterParameter
{
	void Fuzz(const RTC::SCTP::UnrecognizedParameterParameter* parameter);
} // namespace FuzzerRtcSctpUnrecognizedParameterParameter

#endif
