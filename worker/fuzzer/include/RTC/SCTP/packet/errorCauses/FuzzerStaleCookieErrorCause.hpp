#ifndef MS_FUZZER_RTC_SCTP_STALE_COOKIE_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_STALE_COOKIE_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"

namespace FuzzerRtcSctpStaleCookieErrorCause
{
	void Fuzz(const RTC::SCTP::StaleCookieErrorCause* errorCause);
} // namespace FuzzerRtcSctpStaleCookieErrorCause

#endif
