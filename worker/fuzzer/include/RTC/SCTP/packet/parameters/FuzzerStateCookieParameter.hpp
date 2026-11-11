#ifndef MS_FUZZER_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"

namespace FuzzerRtcSctpStateCookieParameter
{
	void Fuzz(const RTC::SCTP::StateCookieParameter* parameter);
} // namespace FuzzerRtcSctpStateCookieParameter

#endif
