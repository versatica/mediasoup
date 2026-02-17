#include "RTC/RTCP/FuzzerFeedbackRtpEcn.hpp"

void FuzzerRtcRtcpFeedbackRtpEcn::Fuzz(RTC::RTCP::FeedbackRtpEcnPacket* packet)
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
		item->GetSequenceNumber();
		item->GetEct0Counter();
		item->GetEct1Counter();
		item->GetEcnCeCounter();
		item->GetNotEctCounter();
		item->GetLostPackets();
		item->GetDuplicatedPackets();
	}
}
