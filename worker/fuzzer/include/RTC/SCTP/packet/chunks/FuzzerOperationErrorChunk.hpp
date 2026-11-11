#ifndef MS_FUZZER_RTC_SCTP_OPERATION_ERROR_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_OPERATION_ERROR_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"

namespace FuzzerRtcSctpOperationErrorChunk
{
	void Fuzz(const RTC::SCTP::OperationErrorChunk* chunk);
} // namespace FuzzerRtcSctpOperationErrorChunk

#endif
