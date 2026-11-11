#ifndef MS_FUZZER_RTC_SCTP_FORWARD_TSN_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"

namespace FuzzerRtcSctpForwardTsnChunk
{
	void Fuzz(const RTC::SCTP::ForwardTsnChunk* chunk);
} // namespace FuzzerRtcSctpForwardTsnChunk

#endif
