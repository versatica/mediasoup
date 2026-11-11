#ifndef MS_FUZZER_RTC_SCTP_INIT_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_INIT_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"

namespace FuzzerRtcSctpInitChunk
{
	void Fuzz(const RTC::SCTP::InitChunk* chunk);
} // namespace FuzzerRtcSctpInitChunk

#endif
