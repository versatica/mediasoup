#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerInitAckChunk.hpp"

void FuzzerRtcSctpInitAckChunk::Fuzz(const RTC::SCTP::InitAckChunk* chunk)
{
	chunk->GetInitiateTag();
	chunk->GetAdvertisedReceiverWindowCredit();
	chunk->GetNumberOfOutboundStreams();
	chunk->GetNumberOfInboundStreams();
	chunk->GetInitialTsn();
}
