#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerDataChunk.hpp"

void FuzzerRtcSctpDataChunk::Fuzz(const RTC::SCTP::DataChunk* chunk)
{
	chunk->GetI();
	chunk->GetU();
	chunk->GetB();
	chunk->GetE();
	chunk->GetTsn();
	chunk->GetStreamId();
	chunk->GetStreamSequenceNumber();
	chunk->GetMessageId();
	chunk->GetPayloadProtocolId();
	chunk->GetFragmentSequenceNumber();
	chunk->HasUserDataPayload();
	chunk->GetUserDataPayload();
	chunk->GetUserDataPayloadLength();
	chunk->MakeUserData();
}
