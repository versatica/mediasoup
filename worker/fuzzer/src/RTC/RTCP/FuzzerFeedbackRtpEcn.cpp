#include "RTC/RTCP/FuzzerFeedbackRtpEcn.hpp"

void Fuzzer::RTC::RTCP::FeedbackRtpEcn::Fuzz(::RTC::RTCP::FeedbackRtpEcnPacket* packet)
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
		item->GetSequenceNumber();
		item->GetEct0Counter();
		item->GetEct1Counter();
		item->GetEcnCeCounter();
		item->GetNotEctCounter();
		item->GetLostPackets();
		item->GetDuplicatedPackets();
	}
}
