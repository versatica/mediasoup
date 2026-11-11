#ifndef MS_FUZZER_RTC_SCTP_HEARTBEAT_ACK_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_HEARTBEAT_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"

namespace FuzzerRtcSctpHeartbeatAckChunk
{
	void Fuzz(const RTC::SCTP::HeartbeatAckChunk* chunk);
} // namespace FuzzerRtcSctpHeartbeatAckChunk

#endif
