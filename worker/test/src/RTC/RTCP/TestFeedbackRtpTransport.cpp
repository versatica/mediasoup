#include "common.hpp"
#include "Logger.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

SCENARIO("RTCP Feeback RTP transport", "[parser][rtcp][feedback-rtp][transport]")
{
	static constexpr size_t RtcpMtu{ 1200u };

	uint32_t senderSsrc{ 1111u };
	uint32_t mediaSsrc{ 2222u };

	SECTION("create FeedbackRtpTransportPacketPacket, run length chunk")
	{
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

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 15);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket, run length chunk (2)")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(2015, 10000000, RtcpMtu);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 1016);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket, mixed chunks")
	{
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

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 18);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacketPacket, incomplete two bit vector chunk")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 10000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 10000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 10002560, RtcpMtu);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 2);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create two sequential FeedbackRtpTransportPacketPackets")
	{
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

		SECTION("parse serialized buffer")
		{
			auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

			REQUIRE(packet2);
			REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
			REQUIRE(packet2->GetPacketStatusCount() == 8);
			REQUIRE(packet2->GetFeedbackPacketCount() == 1);

			uint8_t buffer2[1024];
			auto len2 = packet2->Serialize(buffer2);

			REQUIRE(len == len2);
			REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

			delete packet2;
		}

		auto highestWideSeqNumber = packet->GetHighestSequenceNumber();
		auto highestTimestamp     = packet->GetHighestTimestamp();

		auto* packet2 = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet2->SetFeedbackPacketCount(2);

		packet2->AddPacket(highestWideSeqNumber, highestTimestamp, RtcpMtu);
		packet2->AddPacket(1009, 10000009, RtcpMtu);
		packet2->AddPacket(1010, 10000010, RtcpMtu);
		packet2->AddPacket(1011, 10000011, RtcpMtu);
		packet2->AddPacket(1012, 10000012, RtcpMtu);
		packet2->AddPacket(1013, 10000013, RtcpMtu);
		packet2->AddPacket(1014, 10000014, RtcpMtu);

		len = packet2->Serialize(buffer);

		SECTION("parse serialized buffer")
		{
			auto* packet3 = FeedbackRtpTransportPacket::Parse(buffer, len);

			REQUIRE(packet3);
			REQUIRE(packet3->GetBaseSequenceNumber() == 1008);
			REQUIRE(packet3->GetPacketStatusCount() == 7);
			REQUIRE(packet3->GetFeedbackPacketCount() == 2);

			uint8_t buffer2[1024];
			auto len2 = packet3->Serialize(buffer2);

			REQUIRE(len == len2);
			REQUIRE(std::memcmp(buffer, buffer2, len) == 0);

			delete packet3;
		}

		delete packet2;
		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacketPacket, one bit vector chunk")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x07, 0xFA, 0x17, 0xFA, 0x17, 0x09, 0xFA, 0xFF,
			0x67, 0x00, 0x27, 0x00, 0x0D, 0x5F, 0xC2, 0xF1, 0x03, 0xBF, 0x8E,
			0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x04, 0x00
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, 32);

		REQUIRE(packet);
		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 13);
		REQUIRE(packet->GetFeedbackPacketCount() == 3);
		REQUIRE(packet->GetReferenceTime() == 6275825);

		SECTION("parse serialized buffer")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == 32);
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}
}
