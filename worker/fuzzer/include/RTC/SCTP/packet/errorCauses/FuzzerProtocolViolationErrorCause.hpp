#ifndef MS_FUZZER_RTC_SCTP_PROTOCOL_VIOLATION_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_PROTOCOL_VIOLATION_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"

namespace FuzzerRtcSctpProtocolViolationErrorCause
{
	void Fuzz(const RTC::SCTP::ProtocolViolationErrorCause* errorCause);
} // namespace FuzzerRtcSctpProtocolViolationErrorCause

#endif
