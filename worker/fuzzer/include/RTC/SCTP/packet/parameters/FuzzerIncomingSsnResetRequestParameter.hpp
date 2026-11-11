#ifndef MS_FUZZER_RTC_SCTP_INCOMING_SSN_RESET_REQUEST_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_INCOMING_SSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"

namespace FuzzerRtcSctpIncomingSsnResetRequestParameter
{
	void Fuzz(const RTC::SCTP::IncomingSsnResetRequestParameter* parameter);
} // namespace FuzzerRtcSctpIncomingSsnResetRequestParameter

#endif
