#ifndef MS_FUZZER_RTC_SCTP_INIT_ACK_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_INIT_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"

namespace FuzzerRtcSctpInitAckChunk
{
	void Fuzz(const RTC::SCTP::InitAckChunk* chunk);
} // namespace FuzzerRtcSctpInitAckChunk

#endif
