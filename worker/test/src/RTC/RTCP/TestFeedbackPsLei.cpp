#include "common.hpp"
#include "RTC/RTCP/FeedbackPsLei.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

SCENARIO("RTCP Feedback PS LEI", "[rtcp][feedback-ps][lei]")
{
	// RTCP LEI packet.

	// clang-format off
	uint8_t buffer[] =
	{
		0x88, 0xce, 0x00, 0x03, // Type: 206 (Payload Specific), Count: 8 (LEI), Length: 3
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x02, 0xd0, 0x37, 0x02, // SSRC: 0x02d03702
	};
	// clang-format on

	// LEI values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };
	const uint32_t ssrc{ 0x02d03702 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](FeedbackPsLeiPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		const FeedbackPsLeiItem* item = *(packet->Begin());

		REQUIRE(item->GetSsrc() == ssrc);
	};

	SECTION("parse FeedbackPsLeiPacket")
	{
		std::unique_ptr<FeedbackPsLeiPacket> packet{ FeedbackPsLeiPacket::Parse(buffer, sizeof(buffer)) };

		REQUIRE(packet);

		verify(packet.get());

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}

	SECTION("create FeedbackPsLeiPacket")
	{
		FeedbackPsLeiPacket packet(senderSsrc, mediaSsrc);

		auto* item = new FeedbackPsLeiItem(ssrc);

		packet.AddItem(item);

		verify(&packet);
	}
}
