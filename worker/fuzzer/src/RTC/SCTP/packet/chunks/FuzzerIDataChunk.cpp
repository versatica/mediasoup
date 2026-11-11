#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerIDataChunk.hpp"

void FuzzerRtcSctpIDataChunk::Fuzz(const RTC::SCTP::IDataChunk* chunk)
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
