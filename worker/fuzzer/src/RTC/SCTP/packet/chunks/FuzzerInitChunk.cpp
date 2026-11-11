#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerInitChunk.hpp"

void FuzzerRtcSctpInitChunk::Fuzz(const RTC::SCTP::InitChunk* chunk)
{
	chunk->GetInitiateTag();
	chunk->GetAdvertisedReceiverWindowCredit();
	chunk->GetNumberOfOutboundStreams();
	chunk->GetNumberOfInboundStreams();
	chunk->GetInitialTsn();
}
