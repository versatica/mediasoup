#ifndef MS_FUZZER_RTC_SCTP_UNKNOWN_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_UNKNOWN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"

namespace FuzzerRtcSctpUnknownChunk
{
	void Fuzz(const RTC::SCTP::UnknownChunk* chunk);
} // namespace FuzzerRtcSctpUnknownChunk

#endif
