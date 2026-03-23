#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRpsi.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback PS RPSI", "[rtcp][feedback-ps][rpsi]")
{
	// RTCP RPSI packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x83, 0xce, 0x00, 0x04, // Type: 206 (Payload Specific), Count: 3 (RPSI), Length: 4
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x08,                   // Padding Bits
		      0x02,             // Zero | Payload Type
		            0x00, 0x00, // Native RPSI bit string
		0x00, 0x00, 0x01, 0x00
	};
	// clang-format on

	// RPSI values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };
	const uint8_t payloadType{ 2 };
	const uint8_t payloadMask{ 1 };
	const size_t length{ 5 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::FeedbackPsRpsiPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		const RTC::RTCP::FeedbackPsRpsiItem* item = *(packet->Begin());

		REQUIRE(item);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetBitString()[item->GetLength() - 1] & 1) == payloadMask);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::FeedbackPsRpsiItem::Header) == 1);
	}

	SECTION("parse FeedbackPsRpsiPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackPsRpsiPacket> packet{ RTC::RTCP::FeedbackPsRpsiPacket::Parse(
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
