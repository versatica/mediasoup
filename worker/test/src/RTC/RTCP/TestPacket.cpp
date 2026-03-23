#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("RTCP Packet", "[rtcp][packet]")
{
	// RTCP common header
	// Version:2, Padding:false, Count:0, Type:200(SR), Lengh:0

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x80, 0xc8, 0x00, 0x00
	};
	// clang-format on

	SECTION("alignof() RTCP structs")
	{
		REQUIRE(alignof(RTC::RTCP::Packet::CommonHeader) == 2);
		REQUIRE(alignof(RTC::RTCP::FeedbackRtpPacket::Header) == 4);
		REQUIRE(alignof(RTC::RTCP::FeedbackPsPacket::Header) == 4);
	}

	SECTION("a RTCP packet may only contain the RTCP common header")
	{
		const std::unique_ptr<RTC::RTCP::Packet> packet{ RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)) };

		REQUIRE(packet);
	}

	SECTION("a too small RTCP packet should fail")
	{
		// Provide a wrong packet length.
		const size_t length = sizeof(buffer) - 1;

		const std::unique_ptr<RTC::RTCP::Packet> packet{ RTC::RTCP::Packet::Parse(buffer, length) };

		REQUIRE(!packet);
	}

	SECTION("a RTCP packet with incorrect version should fail")
	{
		// Set an incorrect version value (0).
		buffer[0] &= 0b00111111;

		const std::unique_ptr<RTC::RTCP::Packet> packet{ RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)) };

		REQUIRE(!packet);
	}

	SECTION("a RTCP packet with incorrect length should fail")
	{
		// Set the packet length to zero.
		buffer[3] = 1;

		const std::unique_ptr<RTC::RTCP::Packet> packet{ RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)) };

		REQUIRE(!packet);
	}

	SECTION("a RTCP packet with unknown type should fail")
	{
		// Set and unknown packet type (0).
		buffer[1] = 0;

		const std::unique_ptr<RTC::RTCP::Packet> packet{ RTC::RTCP::Packet::Parse(buffer, sizeof(buffer)) };

		REQUIRE(!packet);
	}
}
