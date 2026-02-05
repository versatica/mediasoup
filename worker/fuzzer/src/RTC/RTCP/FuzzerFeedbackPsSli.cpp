#include "RTC/RTCP/FuzzerFeedbackPsSli.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsSli::Fuzz(::RTC::RTCP::FeedbackPsSliPacket* packet)
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
		item->GetFirst();
		item->SetFirst(1111);
		item->GetNumber();
		item->SetNumber(2222);
		item->GetPictureId();
		item->SetPictureId(255);
	}
}
