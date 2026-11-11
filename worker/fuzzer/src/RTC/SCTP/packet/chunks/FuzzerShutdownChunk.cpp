#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerShutdownChunk.hpp"

void FuzzerRtcSctpShutdownChunk::Fuzz(const RTC::SCTP::ShutdownChunk* chunk)
{
	chunk->GetCumulativeTsnAck();
}
