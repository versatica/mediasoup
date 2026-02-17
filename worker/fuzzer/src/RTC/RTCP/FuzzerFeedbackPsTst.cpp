#include "RTC/RTCP/FuzzerFeedbackPsTst.hpp"

void FuzzerRtcRtcpFeedbackPsTstn::Fuzz(RTC::RTCP::FeedbackPsTstnPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddItem(Item* item);

	for (auto it = packet->Begin(); it != packet->End(); ++it)
	{
		auto& item = (*it);

		item->Serialize(RTC::RTCP::SerializationBuffer);
		item->GetSize();
		item->GetSsrc();
		item->GetSequenceNumber();
		item->GetIndex();
	}
}

void FuzzerRtcRtcpFeedbackPsTstr::Fuzz(RTC::RTCP::FeedbackPsTstrPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddItem(Item* item);

	for (auto it = packet->Begin(); it != packet->End(); ++it)
	{
		auto& item = (*it);

		item->Serialize(RTC::RTCP::SerializationBuffer);
		item->GetSize();
		item->GetSsrc();
		item->GetSequenceNumber();
		item->GetIndex();
	}
}
