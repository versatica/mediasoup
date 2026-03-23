#include "common.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback PS TSTN", "[rtcp][feedback-ps][tstn]")
{
	// RTCP TSTN packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x84, 0xce, 0x00, 0x04, // Type: 206 (Payload Specific), Count: 4 (TST), Length: 4
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x02, 0xd0, 0x37, 0x02, // SSRC: 0x02d03702
		0x08, 0x00, 0x00, 0x01  // Seq: 8, Reserved, Index: 1
	};
	// clang-format on

	// TSTN values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };
	const uint32_t ssrc{ 0x02d03702 };
	const uint8_t seq{ 8 };
	const uint8_t index{ 1 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::FeedbackPsTstnPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		const RTC::RTCP::FeedbackPsTstnItem* item = *(packet->Begin());

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::FeedbackPsTstrItem::Header) == 1);
		REQUIRE(alignof(RTC::RTCP::FeedbackPsTstnItem::Header) == 1);
	}

	SECTION("parse FeedbackPsTstnPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackPsTstnPacket> packet{ RTC::RTCP::FeedbackPsTstnPacket::Parse(
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

	SECTION("create FeedbackPsTstnPacket")
	{
		RTC::RTCP::FeedbackPsTstnPacket packet(senderSsrc, mediaSsrc);

		auto* item = new RTC::RTCP::FeedbackPsTstnItem(ssrc, seq, index);

		packet.AddItem(item);

		verify(&packet);
	}
}
