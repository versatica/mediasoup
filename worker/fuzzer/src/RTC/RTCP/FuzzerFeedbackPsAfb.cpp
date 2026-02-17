#include "RTC/RTCP/FuzzerFeedbackPsAfb.hpp"

void FuzzerRtcRtcpFeedbackPsAfb::Fuzz(RTC::RTCP::FeedbackPsAfbPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetSize();
	packet->GetApplication();
}
