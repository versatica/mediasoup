#ifndef MS_FUZZER_RTC_SCTP_ABORT_ASSOCIATION_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_ABORT_ASSOCIATION_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"

namespace FuzzerRtcSctpAbortAssociationChunk
{
	void Fuzz(const RTC::SCTP::AbortAssociationChunk* chunk);
} // namespace FuzzerRtcSctpAbortAssociationChunk

#endif
