#include "common.hpp"
#include "Logger.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

using namespace RTC::RTCP;

SCENARIO("RTCP Feeback RTP transport", "[parser][rtcp][feedback-rtp][transport]")
{
	static constexpr size_t RtcpMtu{ 1200u };

	SECTION("create FeedbackRtpTransportPacketPacket")
	{
		uint32_t senderSsrc{ 1111u };
		uint32_t mediaSsrc{ 2222u };

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 10000000, RtcpMtu);
		packet->AddPacket(1002, 10000000, RtcpMtu);
		packet->AddPacket(1003, 10000000, RtcpMtu);
		packet->AddPacket(1004, 10000000, RtcpMtu);
		packet->AddPacket(1005, 10000000, RtcpMtu);
		packet->AddPacket(1006, 10000000, RtcpMtu);
		packet->AddPacket(1007, 10000000, RtcpMtu);
		packet->AddPacket(1008, 10000000, RtcpMtu);
		packet->AddPacket(1009, 10000000, RtcpMtu);
		packet->AddPacket(1010, 10000000, RtcpMtu);
		packet->AddPacket(1011, 10000000, RtcpMtu);
		packet->AddPacket(1012, 10000000, RtcpMtu);
		packet->AddPacket(1013, 10000000, RtcpMtu);
		packet->AddPacket(1014, 10000000, RtcpMtu);

		REQUIRE(packet);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			// TODO: Remove.
			// MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			// packet->Dump();
			// MS_DUMP_DATA(buffer, len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 15); // TODO: Fails.
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket (2)")
	{
		uint32_t senderSsrc{ 1111u };
		uint32_t mediaSsrc{ 2222u };

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(2015, 10000000, RtcpMtu);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			// TODO: Remove.
			// MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			// packet->Dump();
			// MS_DUMP_DATA(buffer, len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 1015);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				delete packet2;
			}
		}

		delete packet;
	}
}
