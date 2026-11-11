#ifndef MS_FUZZER_RTC_SCTP_UNKNOWN_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_UNKNOWN_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"

namespace FuzzerRtcSctpUnknownErrorCause
{
	void Fuzz(const RTC::SCTP::UnknownErrorCause* errorCause);
} // namespace FuzzerRtcSctpUnknownErrorCause

#endif
