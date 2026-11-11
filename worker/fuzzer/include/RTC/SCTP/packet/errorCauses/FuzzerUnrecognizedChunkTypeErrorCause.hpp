#ifndef MS_FUZZER_RTC_SCTP_UNRECOGNIZED_CHUNK_TYPE_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_UNRECOGNIZED_CHUNK_TYPE_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"

namespace FuzzerRtcSctpUnrecognizedChunkTypeErrorCause
{
	void Fuzz(const RTC::SCTP::UnrecognizedChunkTypeErrorCause* errorCause);
} // namespace FuzzerRtcSctpUnrecognizedChunkTypeErrorCause

#endif
