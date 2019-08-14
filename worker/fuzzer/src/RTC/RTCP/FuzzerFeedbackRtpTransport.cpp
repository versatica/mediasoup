#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"
#include <cstring> // std::memory()

void Fuzzer::RTC::RTCP::FeedbackRtpTransport::Fuzz(::RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
	static constexpr size_t RtcpMtu{ 2500u };

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
	packet->SetFeedbackPacketCount(0u);

	// We'll increase the packet size, so must clone it into a bigger buffer.
	size_t len2 = packet->GetSize() + 2000u;
	uint8_t data2[len2];

	std::memcpy(data2, packet->GetData(), packet->GetSize());

	auto* packet2 = ::RTC::RTCP::FeedbackRtpTransportPacket::Parse(data2, len2);

	for (uint16_t seq{ 0u }; seq < 30u; ++seq)
	{
		// Generate lost seqs.
		if (seq % 8 == 0)
			continue;

		packet2->AddPacket(seq, 10000000 + (seq * 10), RtcpMtu);
	}

	packet2->Dump();
	packet2->Serialize(::RTC::RTCP::Buffer);
	packet2->GetCount();
	packet2->GetSize();
	packet2->IsFull();
	packet2->IsSerializable();
	packet2->IsCorrect();
	packet2->GetBaseSequenceNumber();
	packet2->GetPacketStatusCount();
	packet2->GetReferenceTime();
	packet2->GetFeedbackPacketCount();
	packet2->GetHighestSequenceNumber();
	packet2->GetHighestTimestamp();
	packet2->SetFeedbackPacketCount(1u);

	delete packet2;
}
