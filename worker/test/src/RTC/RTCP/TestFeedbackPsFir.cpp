#include "common.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback PS FIR", "[rtcp][feedback-ps][fir]")
{
	// RTCP FIR packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x84, 0xce, 0x00, 0x04, // Type: 206 (Payload Specific), Count: 4 (FIR), Length: 4
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x02, 0xd0, 0x37, 0x02, // SSRC: 0x02d03702
		0x04, 0x00, 0x00, 0x00  // Seq: 0x04
	};
	// clang-format on

	// FIR values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };
	const uint32_t ssrc{ 0x02d03702 };
	const uint8_t seq{ 4 };

	// NOTE: No need to pass const integers to the lambda.
	auto verify = [](RTC::RTCP::FeedbackPsFirPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		const RTC::RTCP::FeedbackPsFirItem* item = *(packet->Begin());

		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
	};

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::FeedbackPsFirItem::Header) == 4);
	}

	SECTION("parse FeedbackPsFirPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackPsFirPacket> packet{ RTC::RTCP::FeedbackPsFirPacket::Parse(
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

	SECTION("create FeedbackPsFirPacket")
	{
		RTC::RTCP::FeedbackPsFirPacket packet(senderSsrc, mediaSsrc);

		auto* item = new RTC::RTCP::FeedbackPsFirItem(ssrc, seq);

		packet.AddItem(item);

		verify(&packet);
	}
}
