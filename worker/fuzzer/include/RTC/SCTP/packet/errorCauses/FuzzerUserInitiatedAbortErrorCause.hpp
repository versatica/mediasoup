#ifndef MS_FUZZER_RTC_SCTP_USER_INITIATED_ABORT_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_USER_INITIATED_ABORT_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"

namespace FuzzerRtcSctpUserInitiatedAbortErrorCause
{
	void Fuzz(const RTC::SCTP::UserInitiatedAbortErrorCause* errorCause);
} // namespace FuzzerRtcSctpUserInitiatedAbortErrorCause

#endif
