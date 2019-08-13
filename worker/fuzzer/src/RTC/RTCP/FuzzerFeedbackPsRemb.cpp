#include "RTC/RTCP/FuzzerFeedbackPsRemb.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsRemb::Fuzz(::RTC::RTCP::FeedbackPsRembPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();
	packet->IsCorrect();
	packet->SetBitrate(1111);
	packet->SetSsrcs({ 2222, 3333, 4444 });
	packet->GetBitrate();
	packet->GetSsrcs();
}
