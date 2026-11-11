#ifndef MS_FUZZER_RTC_SCTP_COOKIE_PRESERVATIVE_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_COOKIE_PRESERVATIVE_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"

namespace FuzzerRtcSctpCookiePreservativeParameter
{
	void Fuzz(const RTC::SCTP::CookiePreservativeParameter* parameter);
} // namespace FuzzerRtcSctpCookiePreservativeParameter

#endif
