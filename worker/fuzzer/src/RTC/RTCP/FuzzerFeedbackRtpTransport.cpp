#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

void Fuzzer::RTC::RTCP::FeedbackRtpTransport::Fuzz(::RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
	// packet->Dump();
	packet->GetCount();
	packet->GetSize();
	packet->IsFull();
	packet->IsSerializable();
	packet->IsCorrect();
	packet->GetBaseSequenceNumber();
	packet->GetPacketStatusCount();
	packet->GetReferenceTime();
	packet->GetFeedbackPacketCount();
	packet->GetHighestSequenceNumber();
	packet->GetHighestTimestamp();
	packet->Serialize(::RTC::RTCP::Buffer);
}
