#ifndef MS_FUZZER_RTC_SCTP_INVALID_STREAM_IDENTIFIER_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_INVALID_STREAM_IDENTIFIER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"

namespace FuzzerRtcSctpInvalidStreamIdentifierErrorCause
{
	void Fuzz(const RTC::SCTP::InvalidStreamIdentifierErrorCause* errorCause);
} // namespace FuzzerRtcSctpInvalidStreamIdentifierErrorCause

#endif
