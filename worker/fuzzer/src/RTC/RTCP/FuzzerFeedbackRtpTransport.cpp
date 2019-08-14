#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

	// TODO: REMOVE
	#include "Logger.hpp"
	#define MS_CLASS "FuzzerFeedbackRtpTransport"

void Fuzzer::RTC::RTCP::FeedbackRtpTransport::Fuzz(::RTC::RTCP::FeedbackRtpTransportPacket* packet)
{
	static size_t RtcpMtu{ 1500u };

		MS_DUMP(">>>>>>>>>> dumping original packet:");
	packet->Dump();
	// auto len = packet->Serialize(::RTC::RTCP::Buffer);
		// MS_DUMP_DATA(packet->GetData(), len);
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

	// We'll increase the packet size, so must clone it into a bigger buffer.
	auto* packet2 = ::RTC::RTCP::FeedbackRtpTransportPacket::Parse(::RTC::RTCP::Buffer, packet->GetSize());

	// TODO
	if (!packet2)
		MS_DUMP("------------------- packet2 is nullptr! THIS SHOULD NOT HAPPEN!");

	for (uint16_t seq{ 0u }; seq < 30u; ++seq)
	{
		// Generate lost seqs.
		if (seq % 8 == 0)
			continue;

		// Do not produce an assert.
		if (packet2->IsFull())
			break;

		packet2->AddPacket(seq, 10000000 + (seq * 10), RtcpMtu);
	}

		MS_DUMP(">>>>>>>>>> dumping packet2:");
	packet2->Dump();
	if (packet2->IsSerializable())
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
