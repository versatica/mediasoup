#ifndef MS_FUZZER_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/SsnTsnResetRequestParameter.hpp"

namespace FuzzerRtcSctpSsnTsnResetRequestParameter
{
	void Fuzz(const RTC::SCTP::SsnTsnResetRequestParameter* parameter);
} // namespace FuzzerRtcSctpSsnTsnResetRequestParameter

#endif
