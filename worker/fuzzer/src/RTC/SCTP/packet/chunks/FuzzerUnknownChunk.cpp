#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerUnknownChunk.hpp"

void FuzzerRtcSctpUnknownChunk::Fuzz(const RTC::SCTP::UnknownChunk* chunk)
{
	chunk->HasUnknownValue();
	chunk->GetUnknownValue();
	chunk->GetUnknownValueLength();
}
