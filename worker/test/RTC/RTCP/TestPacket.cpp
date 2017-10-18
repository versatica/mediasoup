#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/Packet.hpp"

using namespace RTC::RTCP;

SCENARIO("RTCP parsing", "[parser][rtcp][packet]")
{
	SECTION("a RTCP packet may only contain the RTCP common header")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x00 // RTCP common header
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		delete packet;
	}

	SECTION("a too small RTCP packet should fail")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("a RTCP packet with incorrect version should fail")
	{
		uint8_t buffer[] =
		{
			0x00, 0xca, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("a RTCP packet with incorrect length should fail")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("a RTCP packet with unknown type should fail")
	{
		uint8_t buffer[] =
		{
			0x81, 0x00, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}
}
