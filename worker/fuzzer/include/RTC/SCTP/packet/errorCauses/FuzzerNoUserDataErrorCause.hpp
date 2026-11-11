#ifndef MS_FUZZER_RTC_SCTP_NO_USER_DATA_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_NO_USER_DATA_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"

namespace FuzzerRtcSctpNoUserDataErrorCause
{
	void Fuzz(const RTC::SCTP::NoUserDataErrorCause* errorCause);
} // namespace FuzzerRtcSctpNoUserDataErrorCause

#endif
