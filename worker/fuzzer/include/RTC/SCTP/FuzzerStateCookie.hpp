#ifndef MS_FUZZER_RTC_SCTP_STATE_COOKIE_HPP
#define MS_FUZZER_RTC_SCTP_STATE_COOKIE_HPP

#include "common.hpp"

namespace FuzzerRtcSctpStateCookie
{
	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcSctpStateCookie

#endif
