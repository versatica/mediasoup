#include "RTC/RTCP/FuzzerFeedbackPsRpsi.hpp"

void FuzzerRtcRtcpFeedbackPsRpsi::Fuzz(RTC::RTCP::FeedbackPsRpsiPacket* packet)
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
		item->IsCorrect();
		item->GetPayloadType();
		item->GetBitString();
		item->GetLength();
	}
}
