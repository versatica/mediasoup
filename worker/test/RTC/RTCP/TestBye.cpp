#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/Bye.hpp"
#include <string>

using namespace RTC::RTCP;

namespace TestBye
{
	// RCTP BYE packet.
	uint8_t buffer[] =
	{
		0x82, 0xcb, 0x00, 0x06, // Type: 203 (Bye), Count: 2, length: 2
		0x62, 0x42, 0x76, 0xe0, // SSRC: 0x624276e0
		0x26, 0x24, 0x67, 0x0e, // SSRC: 0x2624670e
		0x0e, 0x48, 0x61, 0x73, // Length: 14, Text: "Hasta la vista"
		0x74, 0x61, 0x20, 0x6c,
		0x61, 0x20, 0x76, 0x69,
		0x73, 0x74, 0x61, 0x00
	};

	uint32_t ssrc1 = 0x624276e0;
	uint32_t ssrc2 = 0x2624670e;
	std::string reason("Hasta la vista");

	void verifyPacket(ByePacket* packet)
	{
		REQUIRE(packet->GetReason() == reason);

		ByePacket::Iterator it = packet->Begin();

		REQUIRE(*it == ssrc1);

		it++;

		REQUIRE(*it == ssrc2);
	}
}

using namespace TestBye;

SCENARIO("RTCP BYE parsing", "[parser][rtcp][bye]")
{
	SECTION("parse BYE packet")
	{
		ByePacket* packet = ByePacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		verifyPacket(packet);

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = {0};

			packet->Serialize(serialized);

			SECTION("compare serialized instance with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}

		delete packet;
	}

	SECTION("create ByePacket")
	{
		// Create local Bye packet and check content.
		ByePacket packet;

		packet.AddSsrc(ssrc1);
		packet.AddSsrc(ssrc2);
		packet.SetReason(reason);

		verifyPacket(&packet);

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = {0};

			packet.Serialize(serialized);

			SECTION("compare serialized instance with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}
	}
}
