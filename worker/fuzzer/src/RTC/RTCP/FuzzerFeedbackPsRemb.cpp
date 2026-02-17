#include "RTC/RTCP/FuzzerFeedbackPsRemb.hpp"

void FuzzerRtcRtcpFeedbackPsRemb::Fuzz(RTC::RTCP::FeedbackPsRembPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();
	packet->IsCorrect();
	packet->SetBitrate(1111);
	packet->SetSsrcs({ 2222, 3333, 4444 });
	packet->GetBitrate();
	packet->GetSsrcs();
}
