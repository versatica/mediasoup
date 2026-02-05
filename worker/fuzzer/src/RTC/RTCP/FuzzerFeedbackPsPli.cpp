#include "RTC/RTCP/FuzzerFeedbackPsPli.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsPli::Fuzz(::RTC::RTCP::FeedbackPsPliPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();
}
