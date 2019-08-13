#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

void Fuzzer::RTC::RTCP::FeedbackRtpTransport::Fuzz(::RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
	// static constexpr size_t RtcpMtu{ 2500u };

	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
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
	// packet->SetFeedbackPacketCount(0);

	// TODO: This will crash since there is no space in the cloned buffer for them.
	// for (uint16_t seq{ 0u }; seq < 30u; ++seq)
	// {
	// 	// Geenrate lost seqs.
	// 	if (seq % 5 == 0)
	// 		continue;

	// 	packet->AddPacket(seq, 10000000 + (seq * 10), RtcpMtu);
	// }

	// packet->Dump();
	// packet->Serialize(::RTC::RTCP::Buffer);
	// packet->GetCount();
	// packet->GetSize();
	// packet->IsFull();
	// packet->IsSerializable();
	// packet->IsCorrect();
	// packet->GetBaseSequenceNumber();
	// packet->GetPacketStatusCount();
	// packet->GetReferenceTime();
	// packet->GetFeedbackPacketCount(1);
	// packet->GetHighestSequenceNumber();
	// packet->GetHighestTimestamp();
	// packet->SetFeedbackPacketCount();
}
