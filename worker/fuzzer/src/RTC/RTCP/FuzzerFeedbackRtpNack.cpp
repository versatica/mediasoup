#include "RTC/RTCP/FuzzerFeedbackRtpNack.hpp"

void FuzzerRtcRtcpFeedbackRtpNack::Fuzz(RTC::RTCP::FeedbackRtpNackPacket* packet)
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
		item->GetPacketId();
		item->GetLostPacketBitmask();
	}
}
