#ifndef MS_FUZZER_RTC_SCTP_SACK_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_SACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"

namespace FuzzerRtcSctpSackChunk
{
	void Fuzz(const RTC::SCTP::SackChunk* chunk);
} // namespace FuzzerRtcSctpSackChunk

#endif
