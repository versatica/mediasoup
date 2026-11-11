#ifndef MS_FUZZER_RTC_SCTP_OUTGOING_SSN_RESET_REQUEST_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_OUTGOING_SSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"

namespace FuzzerRtcSctpOutgoingSsnResetRequestParameter
{
	void Fuzz(const RTC::SCTP::OutgoingSsnResetRequestParameter* parameter);
} // namespace FuzzerRtcSctpOutgoingSsnResetRequestParameter

#endif
