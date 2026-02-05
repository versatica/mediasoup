#include "RTC/RTCP/FuzzerFeedbackPsTst.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsTstn::Fuzz(::RTC::RTCP::FeedbackPsTstnPacket* packet)
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
		item->GetIndex();
	}
}

void Fuzzer::RTC::RTCP::FeedbackPsTstr::Fuzz(::RTC::RTCP::FeedbackPsTstrPacket* packet)
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
		item->GetIndex();
	}
}
