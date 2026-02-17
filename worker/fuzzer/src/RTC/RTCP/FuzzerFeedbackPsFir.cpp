#include "RTC/RTCP/FuzzerFeedbackPsFir.hpp"

void FuzzerRtcRtcpFeedbackPsFir::Fuzz(RTC::RTCP::FeedbackPsFirPacket* packet)
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
	}
}
