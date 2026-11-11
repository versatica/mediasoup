#ifndef MS_FUZZER_RTC_SCTP_I_FORWARD_TSN_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_I_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"

namespace FuzzerRtcSctpIForwardTsnChunk
{
	void Fuzz(const RTC::SCTP::IForwardTsnChunk* chunk);
} // namespace FuzzerRtcSctpIForwardTsnChunk

#endif
