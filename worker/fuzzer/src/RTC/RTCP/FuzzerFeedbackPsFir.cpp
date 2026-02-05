#include "RTC/RTCP/FuzzerFeedbackPsFir.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsFir::Fuzz(::RTC::RTCP::FeedbackPsFirPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddItem(Item* item);

	for (auto it = packet->Begin(); it != packet->End(); ++it)
	{
		auto& item = (*it);

		// item->Dump();
		item->Serialize(::RTC::RTCP::SerializationBuffer);
		item->GetSize();
		item->GetSsrc();
		item->GetSequenceNumber();
	}
}
