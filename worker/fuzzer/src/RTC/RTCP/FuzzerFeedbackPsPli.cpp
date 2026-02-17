#include "RTC/RTCP/FuzzerFeedbackPsPli.hpp"

void FuzzerRtcRtcpFeedbackPsPli::Fuzz(RTC::RTCP::FeedbackPsPliPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();
}
