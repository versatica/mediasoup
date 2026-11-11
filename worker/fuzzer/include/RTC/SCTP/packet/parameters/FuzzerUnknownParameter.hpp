#ifndef MS_FUZZER_RTC_SCTP_UNKNOWN_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_UNKNOWN_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"

namespace FuzzerRtcSctpUnknownParameter
{
	void Fuzz(const RTC::SCTP::UnknownParameter* parameter);
} // namespace FuzzerRtcSctpUnknownParameter

#endif
