#ifndef MS_FUZZER_RTC_SCTP_SHUTDOWN_COMPLETE_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_SHUTDOWN_COMPLETE_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"

namespace FuzzerRtcSctpShutdownCompleteChunk
{
	void Fuzz(const RTC::SCTP::ShutdownCompleteChunk* chunk);
} // namespace FuzzerRtcSctpShutdownCompleteChunk

#endif
