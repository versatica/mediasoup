#include "common.hpp"
#include "RTC/RTCP/FeedbackPsVbcm.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback PS VBCM", "[rtcp][feedback-ps][vbcm]")
{
	// RTCP VBCM packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x84, 0xce, 0x00, 0x05, // Type: 206 (Payload Specific), Count: 4 (VBCM), Length: 5
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x02, 0xd0, 0x37, 0x02, // SSRC: 0x02d03702
		0x08,                   // Seq: 8
		      0x02,             // Zero | Payload Vbcm
		            0x00, 0x01, // Length
		0x01,                   // VBCM Octet String
		      0x00, 0x00, 0x00  // Padding
	};
	// clang-format on

	// VBCM values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };
	const uint32_t ssrc{ 0x02d03702 };
	const uint8_t seq{ 8 };
	const uint8_t payloadType{ 2 };
	const uint16_t length{ 1 };
	const uint8_t valueMask{ 1 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::FeedbackPsVbcmPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		const RTC::RTCP::FeedbackPsVbcmItem* item = *(packet->Begin());

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetValue()[item->GetLength() - 1] & 1) == valueMask);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::FeedbackPsVbcmItem::Header) == 4);
	}

	SECTION("parse FeedbackPsVbcmPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackPsVbcmPacket> packet{ RTC::RTCP::FeedbackPsVbcmPacket::Parse(
			buffer, sizeof(buffer)) };

		REQUIRE(packet);

		verify(packet.get());

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}
}
