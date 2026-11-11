#ifndef MS_FUZZER_RTC_SCTP_RE_CONFIG_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_RE_CONFIG_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"

namespace FuzzerRtcSctpReConfigChunk
{
	void Fuzz(const RTC::SCTP::ReConfigChunk* chunk);
} // namespace FuzzerRtcSctpReConfigChunk

#endif
