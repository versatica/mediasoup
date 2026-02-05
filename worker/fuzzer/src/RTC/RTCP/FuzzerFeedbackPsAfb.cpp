#include "RTC/RTCP/FuzzerFeedbackPsAfb.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsAfb::Fuzz(::RTC::RTCP::FeedbackPsAfbPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::SerializationBuffer);
	packet->GetSize();
	packet->GetApplication();
}
