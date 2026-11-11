#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerForwardTsnChunk.hpp"

void FuzzerRtcSctpForwardTsnChunk::Fuzz(const RTC::SCTP::ForwardTsnChunk* chunk)
{
	chunk->GetNewCumulativeTsn();
	chunk->GetNumberOfSkippedStreams();
	chunk->GetSkippedStreams();
}
