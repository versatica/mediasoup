#include "common.hpp"
#include "RTC/RTCP/Bye.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp()
#include <string>

SCENARIO("RTCP BYE", "[rtcp][bye]")
{
	// RCTP BYE packet.

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x82, 0xcb, 0x00, 0x06, // Type: 203 (Bye), Count: 2, length: 2
		0x62, 0x42, 0x76, 0xe0, // SSRC: 0x624276e0
		0x26, 0x24, 0x67, 0x0e, // SSRC: 0x2624670e
		0x0e, 0x48, 0x61, 0x73, // Length: 14, Text: "Hasta la vista"
		0x74, 0x61, 0x20, 0x6c,
		0x61, 0x20, 0x76, 0x69,
		0x73, 0x74, 0x61, 0x00
	};
	// clang-format on

	const uint32_t ssrc1{ 0x624276e0 };
	const uint32_t ssrc2{ 0x2624670e };
	const std::string reason("Hasta la vista");

	// NOTE: No need to pass const integers to the lambda.
	// NOTE: If we pass const integers then clang-tidy complains with
	// 'clang-diagnostic-unused-lambda-capture'.
	auto verify = [&reason](RTC::RTCP::ByePacket* packet)
	{
		REQUIRE(packet->GetReason() == reason);

		auto it = packet->Begin();

		REQUIRE(*it == ssrc1);

		++it;

		REQUIRE(*it == ssrc2);
	};

	SECTION("parse ByePacket")
	{
		std::unique_ptr<RTC::RTCP::ByePacket> packet{ RTC::RTCP::ByePacket::Parse(buffer, sizeof(buffer)) };

		REQUIRE(packet);

		verify(packet.get());

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized instance with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}

	SECTION("create ByePacket")
	{
		// Create local Bye packet and check content.
		RTC::RTCP::ByePacket packet;

		packet.AddSsrc(ssrc1);
		packet.AddSsrc(ssrc2);
		packet.SetReason(reason);

		verify(&packet);

		SECTION("serialize packet instance")
		{
			alignas(4) uint8_t serialized[sizeof(buffer)] = { 0 };

			packet.Serialize(serialized);

			SECTION("compare serialized instance with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}
}
