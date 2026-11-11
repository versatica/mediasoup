#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerSackChunk.hpp"

void FuzzerRtcSctpSackChunk::Fuzz(const RTC::SCTP::SackChunk* chunk)
{
	chunk->GetCumulativeTsnAck();
	chunk->GetAdvertisedReceiverWindowCredit();
	chunk->GetGapAckBlocks();
	chunk->GetValidatedGapAckBlocks();
	chunk->GetDuplicateTsns();
}
