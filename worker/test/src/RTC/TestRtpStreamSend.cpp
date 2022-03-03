#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamSend.hpp"
#include <catch2/catch.hpp>
#include <vector>

using namespace RTC;

SCENARIO("NACK and RTP packets retransmission", "[rtp][rtcp][nack]")
{
	class TestRtpStreamListener : public RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(RtpStreamSend* /*rtpStream*/, RtpPacket* packet) override
		{
			this->retransmittedPackets.push_back(packet);
		}

	public:
		std::vector<RtpPacket*> retransmittedPackets;
	};

	TestRtpStreamListener testRtpStreamListener;

	SECTION("receive NACK and get retransmitted packets")
	{
		// clang-format off
		uint8_t rtpBuffer1[] =
		{
			0b10000000, 0b01111011, 0b01010010, 0b00001110,
			0b01011011, 0b01101011, 0b11001010, 0b10110101,
			0, 0, 0, 2
		};
		// clang-format on

		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1 = RtpPacket::Parse(rtpBuffer1, sizeof(rtpBuffer1));

		REQUIRE(packet1);
		REQUIRE(packet1->GetSequenceNumber() == 21006);
		REQUIRE(packet1->GetTimestamp() == 1533790901);

		// packet2 [pt:123, seq:21007, timestamp:1533791173]
		auto packet2 = packet1->Clone();

		packet2->SetSequenceNumber(21007);
		packet2->SetTimestamp(1533791173);

		REQUIRE(packet2->GetSequenceNumber() == 21007);
		REQUIRE(packet2->GetTimestamp() == 1533791173);

		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3 = packet1->Clone();

		packet3->SetSequenceNumber(21008);
		packet3->SetTimestamp(1533793871);

		REQUIRE(packet3->GetSequenceNumber() == 21008);
		REQUIRE(packet3->GetTimestamp() == 1533793871);

		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4 = packet1->Clone();

		packet4->SetSequenceNumber(21009);
		packet4->SetTimestamp(1533793871);

		REQUIRE(packet4->GetSequenceNumber() == 21009);
		REQUIRE(packet4->GetTimestamp() == 1533793871);

		// packet5 [pt:123, seq:21010, timestamp:1533971078]
		auto packet5 = packet1->Clone();

		packet5->SetSequenceNumber(21010);
		packet5->SetTimestamp(1533971078);

		REQUIRE(packet5->GetSequenceNumber() == 21010);
		REQUIRE(packet5->GetTimestamp() == 1533971078);

		RtpStream::Params params;

		params.ssrc      = packet1->GetSsrc();
		params.clockRate = 90000;
		params.useNack   = true;

		// Create a RtpStreamSend.
		std::string mid{ "" };
		RtpStreamSend* stream = new RtpStreamSend(&testRtpStreamListener, params, mid, true);

		// Receive all the packets (some of them not in order and/or duplicated).
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet1.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet3.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet2.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet3.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet4.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet4.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet5.get(), clonedPacket);
		}
		{
			RtpPacket::SharedPtr clonedPacket{ nullptr };
			stream->ReceivePacket(packet5.get(), clonedPacket);
		}

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(packet1->GetSequenceNumber(), 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == packet1->GetSequenceNumber());
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 4);

		auto rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];
		auto rtxPacket3 = testRtpStreamListener.retransmittedPackets[2];
		auto rtxPacket4 = testRtpStreamListener.retransmittedPackets[3];

		testRtpStreamListener.retransmittedPackets.clear();

		REQUIRE(rtxPacket1);
		REQUIRE(rtxPacket1->GetSequenceNumber() == packet2->GetSequenceNumber());
		REQUIRE(rtxPacket1->GetTimestamp() == packet2->GetTimestamp());

		REQUIRE(rtxPacket2);
		REQUIRE(rtxPacket2->GetSequenceNumber() == packet3->GetSequenceNumber());
		REQUIRE(rtxPacket2->GetTimestamp() == packet3->GetTimestamp());

		REQUIRE(rtxPacket3);
		REQUIRE(rtxPacket3->GetSequenceNumber() == packet4->GetSequenceNumber());
		REQUIRE(rtxPacket3->GetTimestamp() == packet4->GetTimestamp());

		REQUIRE(rtxPacket4);
		REQUIRE(rtxPacket4->GetSequenceNumber() == packet5->GetSequenceNumber());
		REQUIRE(rtxPacket4->GetTimestamp() == packet5->GetTimestamp());

		delete stream;
	}

	SECTION("Cloned RTP packet differs from original, get retransmitted packets")
	{
		// clang-format off
		uint8_t rtpBuffer1[] =
		{
			0b10000000, 0b01111011, 0b01010010, 0b00001110,
			0b01011011, 0b01101011, 0b11001010, 0b10110101,
			0, 0, 0, 2
		};
		// clang-format on

		RtpStream::Params params;

		params.ssrc      = 1111;
		params.clockRate = 90000;
		params.useNack   = true;

		// Create a RtpStreamSend.
		std::string mid{ "" };
		RtpStreamSend* stream = new RtpStreamSend(&testRtpStreamListener, params, mid, true);

		auto packet = RtpPacket::Parse(rtpBuffer1, sizeof(rtpBuffer1));

		REQUIRE(packet);

		// Original packet.
		packet->SetSsrc(1111);
		packet->SetSequenceNumber(2222);
		packet->SetTimestamp(3333);

		auto clonedPacket = packet->Clone();

		REQUIRE(clonedPacket);

		// Cloned packet.
		clonedPacket->SetSsrc(4444);
		clonedPacket->SetSequenceNumber(5555);
		clonedPacket->SetTimestamp(6666);

		stream->ReceivePacket(packet.get(), clonedPacket);

		// Create a NACK item that requests the packet.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(packet->GetSequenceNumber(), 0b0000000000000000);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == packet->GetSequenceNumber());
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000000);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 1);

		auto rtxPacket = testRtpStreamListener.retransmittedPackets[0];

		testRtpStreamListener.retransmittedPackets.clear();

		// Make sure RTX packets correspond match the origina packet info.
		REQUIRE(rtxPacket);
		REQUIRE(rtxPacket->GetSequenceNumber() == 2222);
		REQUIRE(rtxPacket->GetTimestamp() == 3333);

		delete stream;
	}
}
