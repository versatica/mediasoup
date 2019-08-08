#include "common.hpp"
#include "Logger.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

// TODO: Remove.
#define MS_CLASS "TestFeedbackTransport"

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
			MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			packet->Dump();
			MS_DUMP_DATA(buffer, len);

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
			MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			packet->Dump();
			MS_DUMP_DATA(buffer, len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 1016);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket (3)")
	{
		uint32_t senderSsrc{ 1111u };
		uint32_t mediaSsrc{ 2222u };

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 10000000, RtcpMtu);
		packet->AddPacket(1002, 10000000, RtcpMtu);
		packet->AddPacket(1015, 10000000, RtcpMtu);
		packet->AddPacket(1016, 10000000, RtcpMtu);
		packet->AddPacket(1017, 10000000, RtcpMtu);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			// TODO: Remove.
			MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			packet->Dump();
			MS_DUMP_DATA(buffer, len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 18);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket (4)")
	{
		uint32_t senderSsrc{ 1111u };
		uint32_t mediaSsrc{ 2222u };

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 10002560, RtcpMtu);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			// TODO: Remove.
			MS_DUMP("len: %zu, packet size: %zu", len, packet->GetSize());
			packet->Dump();
			MS_DUMP_DATA(buffer, len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 2);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket (5)")
	{
		uint32_t senderSsrc{ 1111u };
		uint32_t mediaSsrc{ 2222u };

		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 10000001, RtcpMtu);
		packet->AddPacket(1002, 10000002, RtcpMtu);
		packet->AddPacket(1003, 10000003, RtcpMtu);
		packet->AddPacket(1004, 10000004, RtcpMtu);
		packet->AddPacket(1005, 10000005, RtcpMtu);
		packet->AddPacket(1006, 10000006, RtcpMtu);
		packet->AddPacket(1007, 10000007, RtcpMtu);

		uint8_t buffer[1024];
		auto len = packet->Serialize(buffer);

		packet->Dump();
		MS_DUMP_DATA(buffer, len);

		auto lastWideSeqNumber = packet->GetLastSequenceNumber();
		auto lastTimestamp     = packet->GetLastTimestamp();

		auto* packet2 = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);
		packet2->SetFeedbackPacketCount(2);

		packet2->AddPacket(lastWideSeqNumber, lastTimestamp, RtcpMtu);
		packet2->AddPacket(1009, 10000009, RtcpMtu);
		packet2->AddPacket(1010, 10000010, RtcpMtu);
		packet2->AddPacket(1011, 10000011, RtcpMtu);
		packet2->AddPacket(1012, 10000012, RtcpMtu);
		packet2->AddPacket(1013, 10000013, RtcpMtu);
		packet2->AddPacket(1014, 10000014, RtcpMtu);

		len = packet2->Serialize(buffer);

		packet2->Dump();
		MS_DUMP_DATA(buffer, len);

		SECTION("parse serialized buffer")
		{
			auto* packet3 = FeedbackRtpTransportPacket::Parse(buffer, len);

			REQUIRE(packet3);
			REQUIRE(packet3->GetBaseSequenceNumber() == 1008);
			REQUIRE(packet3->GetPacketStatusCount() == 7);
			REQUIRE(packet3->GetFeedbackPacketCount() == 2);

			delete packet3;
		}

		delete packet2;
		delete packet;
	}
}
