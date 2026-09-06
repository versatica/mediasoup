#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

void FuzzerRtcRtcpFeedbackRtpTransport::Fuzz(RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
	packet->GetCount();
	packet->GetSize();
	packet->IsFull();
	packet->IsSerializable();
	packet->IsCorrect();
	packet->GetBaseSequenceNumber();
	packet->GetPacketStatusCount();
	packet->GetReferenceTime();
	packet->GetReferenceTimestampUs();
	packet->GetFeedbackPacketCount();
	packet->GetLatestSequenceNumber();
	packet->GetLatestTimestampUs();
	packet->GetPacketStatuses();
	packet->GetPacketFractionLost();
	packet->Serialize(RTC::RTCP::SerializationBuffer);
}
