#include "RTC/RTCP/FuzzerFeedbackPsPli.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsPli::Fuzz(::RTC::RTCP::FeedbackPsPliPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();
}
