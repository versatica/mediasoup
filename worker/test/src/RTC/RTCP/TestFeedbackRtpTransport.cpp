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

	SECTION(
	  "create FeedbackRtpTransportPacket, small delta run length chunk and single large delta status packet")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		REQUIRE(packet);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 1000000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 1000000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 1000000001, RtcpMtu);
		packet->AddPacket(1002, 1000000012, RtcpMtu);
		packet->AddPacket(1003, 1000000015, RtcpMtu);
		packet->AddPacket(1004, 1000000017, RtcpMtu);
		packet->AddPacket(1005, 1000000018, RtcpMtu);
		packet->AddPacket(1006, 1000000018, RtcpMtu);
		packet->AddPacket(1007, 1000000018, RtcpMtu);
		packet->AddPacket(1008, 1000000018, RtcpMtu);
		packet->AddPacket(1009, 1000000019, RtcpMtu);
		packet->AddPacket(1010, 1000000010, RtcpMtu);
		packet->AddPacket(1011, 1000000011, RtcpMtu);
		packet->AddPacket(1012, 1000000011, RtcpMtu);
		packet->AddPacket(1013, 1000000013, RtcpMtu);

		REQUIRE(packet->GetLatestSequenceNumber() == 1013);
		REQUIRE(packet->GetLatestTimestamp() == 1000000013);

		// Add a packet with greater seq number but older timestamp.
		packet->AddPacket(1014, 1000000013 - 128, RtcpMtu);

		REQUIRE(packet->GetLatestSequenceNumber() == 1014);
		REQUIRE(packet->GetLatestTimestamp() == 1000000013 - 128);

		packet->AddPacket(1015, 1000000015, RtcpMtu);

		REQUIRE(packet->GetLatestSequenceNumber() == 1015);
		REQUIRE(packet->GetLatestTimestamp() == 1000000015);

		packet->Finish();

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 16);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);

			// TODO
			printf("packet->Dump() 1a\n");
			packet->Dump();

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 16);
				REQUIRE(packet2->GetFeedbackPacketCount() == 1);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

					// TODO
					printf("packet2->Dump() 1b\n");
					packet2->Dump();

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, run length chunk (2)")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(10);
		packet->AddPacket(999, 1000000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 1000000000, RtcpMtu); // Base.
		packet->AddPacket(1050, 1000000216, RtcpMtu);

		packet->Finish();

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 51);
		REQUIRE(packet->GetFeedbackPacketCount() == 10);
		REQUIRE(packet->GetLatestSequenceNumber() == 1050);
		REQUIRE(packet->GetLatestTimestamp() == 1000000216);

			// TODO
			printf("packet->Dump() 2a\n");
			packet->Dump();

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

			SECTION("parse serialized buffer")
			{
				auto* packet2 = FeedbackRtpTransportPacket::Parse(buffer, len);

				REQUIRE(packet2);
				REQUIRE(packet2->GetBaseSequenceNumber() == 1000);
				REQUIRE(packet2->GetPacketStatusCount() == 51);
				REQUIRE(packet2->GetFeedbackPacketCount() == 10);

				uint8_t buffer2[1024];
				auto len2 = packet2->Serialize(buffer2);

				REQUIRE(len == len2);
				REQUIRE(std::memcmp(buffer, buffer2, len) == 0);
				REQUIRE(packet2->GetSize() == len2);

					// TODO
					printf("packet2->Dump() 2b\n");
					packet2->Dump();

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, mixed chunks")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 1000000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 1000000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 1000000100, RtcpMtu);
		packet->AddPacket(1002, 1000000200, RtcpMtu);
		packet->AddPacket(1015, 1000000300, RtcpMtu);
		packet->AddPacket(1016, 1000000400, RtcpMtu);
		packet->AddPacket(1017, 1000000500, RtcpMtu);

		packet->Finish();

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 18);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetLatestSequenceNumber() == 1017);
		REQUIRE(packet->GetLatestTimestamp() == 1000000500);

			// TODO
			printf("packet->Dump() 3a\n");
			packet->Dump();

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

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
				REQUIRE(packet2->GetSize() == len2);

					// TODO
					printf("packet2->Dump() 3b\n");
					packet2->Dump();

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpTransportPacket, incomplete two bit vector chunk")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 1000000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 1000000100, RtcpMtu); // Base.
		packet->AddPacket(1001, 1000000700, RtcpMtu);

		packet->Finish();

			// TODO
			printf("packet->Dump() 4a\n");
			packet->Dump();

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetLatestSequenceNumber() == 1001);
		REQUIRE(packet->GetLatestTimestamp() == 1000000700);

		SECTION("serialize packet instance")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(packet->GetSize() == len);

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
				REQUIRE(packet2->GetSize() == len2);

					// TODO
					printf("packet2->Dump() 4b\n");
					packet2->Dump();

				delete packet2;
			}
		}

		delete packet;
	}

	SECTION("create two sequential FeedbackRtpTransportPackets")
	{
		auto* packet = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet->SetFeedbackPacketCount(1);
		packet->AddPacket(999, 1000000000, RtcpMtu);  // Pre base.
		packet->AddPacket(1000, 1000000000, RtcpMtu); // Base.
		packet->AddPacket(1001, 1000000003, RtcpMtu);
		packet->AddPacket(1002, 1000000003, RtcpMtu);
		packet->AddPacket(1003, 1000000003, RtcpMtu);
		packet->AddPacket(1004, 1000000004, RtcpMtu);
		packet->AddPacket(1005, 1000000005, RtcpMtu);
		packet->AddPacket(1006, 1000000005, RtcpMtu);
		packet->AddPacket(1007, 1000000007, RtcpMtu);

		packet->Finish();

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 8);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetLatestSequenceNumber() == 1007);
		REQUIRE(packet->GetLatestTimestamp() == 1000000007);

			// TODO
			printf("packet->Dump() 5a\n");
			packet->Dump();

		uint8_t buffer[1024];
		auto len = packet->Serialize(buffer);

		REQUIRE(packet->GetSize() == len);

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
			REQUIRE(packet2->GetSize() == len2);

				// TODO
				printf("packet2->Dump() 5b\n");
				packet2->Dump();

			delete packet2;
		}

		auto latestWideSeqNumber = packet->GetLatestSequenceNumber();
		auto latestTimestamp     = packet->GetLatestTimestamp();
		auto* packet2            = new FeedbackRtpTransportPacket(senderSsrc, mediaSsrc);

		packet2->SetFeedbackPacketCount(2);

		packet2->AddPacket(latestWideSeqNumber, latestTimestamp, RtcpMtu);
		packet2->AddPacket(1008, 1000000008, RtcpMtu);
		packet2->AddPacket(1009, 1000000009, RtcpMtu);
		packet2->AddPacket(1010, 1000000010, RtcpMtu);
		packet2->AddPacket(1011, 1000000010, RtcpMtu);
		packet2->AddPacket(1012, 1000000010, RtcpMtu);
		packet2->AddPacket(1013, 1000000014, RtcpMtu);
		packet2->AddPacket(1014, 1000000014, RtcpMtu);

		packet2->Finish();

		REQUIRE(packet2->GetBaseSequenceNumber() == 1008);
		REQUIRE(packet2->GetPacketStatusCount() == 7);
		REQUIRE(packet2->GetFeedbackPacketCount() == 2);
		REQUIRE(packet2->GetLatestSequenceNumber() == 1014);
		REQUIRE(packet2->GetLatestTimestamp() == 1000000014);

			// TODO
			printf("packet2->Dump() 5c\n");
			packet2->Dump();

		len = packet2->Serialize(buffer);

		REQUIRE(packet2->GetSize() == len);

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
			REQUIRE(packet3->GetSize() == len2);

				// TODO
				printf("packet3->Dump() 5d\n");
				packet3->Dump();

			delete packet3;
		}

		delete packet2;
		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacket, one bit vector chunk")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x07,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x0D,
			0x5F, 0xC2, 0xF1, 0x03,
			0xBF, 0x8E, 0x10, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x1C, 0x04, 0x00
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, sizeof(data));

		REQUIRE(packet);
		REQUIRE(packet->GetSize() == sizeof(data));
		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 13);
		REQUIRE(packet->GetReferenceTime() == 6275825); // 0x5FC2F1 (signed 24 bits)
		REQUIRE(packet->GetFeedbackPacketCount() == 3);

			// TODO
			printf("packet->Dump() 6a\n");
			packet->Dump();

		SECTION("serialize packet")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == sizeof(data));
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}

	SECTION("parse FeedbackRtpTransportPacket with negative reference time")
	{
		// clang-format off
		uint8_t data[] =
		{
			0x8F, 0xCD, 0x00, 0x04,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x00,
			0xFF, 0xFF, 0xFE, 0x01
		};
		// clang-format on

		auto* packet = FeedbackRtpTransportPacket::Parse(data, sizeof(data));

		REQUIRE(packet);
		REQUIRE(packet->GetSize() == sizeof(data));
		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 0);
		REQUIRE(packet->GetReferenceTime() == -2); // 0xFFFFFE (signed 24 bits)
		REQUIRE(packet->GetFeedbackPacketCount() == 1);

			// TODO
			printf("packet->Dump() 7a\n");
			packet->Dump();

		SECTION("serialize packet")
		{
			uint8_t buffer[1024];
			auto len = packet->Serialize(buffer);

			REQUIRE(len == sizeof(data));
			REQUIRE(std::memcmp(data, buffer, len) == 0);
		}

		delete packet;
	}
}
