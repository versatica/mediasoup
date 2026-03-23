#include "common.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()

SCENARIO("RTCP Feedback PS AFB", "[rtcp][feedback-ps][afb]")
{
	// RTCP AFB packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x8f, 0xce, 0x00, 0x03, // Type: 206 (Payload Specific), Count: 15 (AFB) Length: 3
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x00, 0x00, 0x00, 0x01  // Data
	};
	// clang-format on

	// AFB values.
	const uint32_t senderSsrc{ 0xfa17fa17 };
	const uint32_t mediaSsrc{ 0 };

	auto verify = [](RTC::RTCP::FeedbackPsAfbPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);
		REQUIRE(packet->GetApplication() == RTC::RTCP::FeedbackPsAfbPacket::Application::UNKNOWN);
	};

	SECTION("parse FeedbackPsAfbPacket")
	{
		std::unique_ptr<RTC::RTCP::FeedbackPsAfbPacket> packet{ RTC::RTCP::FeedbackPsAfbPacket::Parse(
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
