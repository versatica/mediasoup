#ifndef MS_FUZZER_RTC_SCTP_HEARTBEAT_REQUEST_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_HEARTBEAT_REQUEST_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"

namespace FuzzerRtcSctpHeartbeatRequestChunk
{
	void Fuzz(const RTC::SCTP::HeartbeatRequestChunk* chunk);
} // namespace FuzzerRtcSctpHeartbeatRequestChunk

#endif
