#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerAbortAssociationChunk.hpp"

void FuzzerRtcSctpAbortAssociationChunk::Fuzz(const RTC::SCTP::AbortAssociationChunk* chunk)
{
	chunk->GetT();
}
