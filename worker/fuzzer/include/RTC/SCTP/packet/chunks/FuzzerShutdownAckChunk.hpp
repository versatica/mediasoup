#ifndef MS_FUZZER_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"

namespace FuzzerRtcSctpShutdownAckChunk
{
	void Fuzz(const RTC::SCTP::ShutdownAckChunk* chunk);
} // namespace FuzzerRtcSctpShutdownAckChunk

#endif
