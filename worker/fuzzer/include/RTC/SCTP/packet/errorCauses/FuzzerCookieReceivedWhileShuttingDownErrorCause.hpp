#ifndef MS_FUZZER_RTC_SCTP_COOKIE_RECEIVED_WHILE_SHUTTING_DOWN_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_COOKIE_RECEIVED_WHILE_SHUTTING_DOWN_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"

namespace FuzzerRtcSctpCookieReceivedWhileShuttingDownErrorCause
{
	void Fuzz(const RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause* errorCause);
} // namespace FuzzerRtcSctpCookieReceivedWhileShuttingDownErrorCause

#endif
