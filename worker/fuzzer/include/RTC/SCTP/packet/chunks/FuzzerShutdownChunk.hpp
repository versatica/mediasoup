#ifndef MS_FUZZER_RTC_SCTP_SHUTDOWN_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_SHUTDOWN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"

namespace FuzzerRtcSctpShutdownChunk
{
	void Fuzz(const RTC::SCTP::ShutdownChunk* chunk);
} // namespace FuzzerRtcSctpShutdownChunk

#endif
