#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerShutdownCompleteChunk.hpp"

void FuzzerRtcSctpShutdownCompleteChunk::Fuzz(const RTC::SCTP::ShutdownCompleteChunk* chunk)
{
	chunk->GetT();
}
