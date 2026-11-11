#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerIForwardTsnChunk.hpp"

void FuzzerRtcSctpIForwardTsnChunk::Fuzz(const RTC::SCTP::IForwardTsnChunk* chunk)
{
	chunk->GetNewCumulativeTsn();
	chunk->GetNumberOfSkippedStreams();
	chunk->GetSkippedStreams();
}
