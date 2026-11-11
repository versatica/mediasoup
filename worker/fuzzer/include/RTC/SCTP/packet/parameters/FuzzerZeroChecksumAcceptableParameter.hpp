#ifndef MS_FUZZER_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace FuzzerRtcSctpZeroChecksumAcceptableParameter
{
	void Fuzz(const RTC::SCTP::ZeroChecksumAcceptableParameter* parameter);
} // namespace FuzzerRtcSctpZeroChecksumAcceptableParameter

#endif
