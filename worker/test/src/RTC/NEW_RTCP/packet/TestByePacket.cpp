#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/ByePacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("RTCP Bye Packet (203)", "[serializable][rtcp]")
{
	rtcpCommon::ResetBuffers();

	SECTION("ByePacket::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, SC=3, PT:203, Length: 5
			0b10000011, 0xCB, 0x00, 0x05,
			// SSRC 1: 1111111
			0x00, 0x10, 0xF4, 0x47,
			// SSRC 2: 2222222
			0x00, 0x21, 0xE8, 0x8E,
			// SSRC 3: 12345678
			0x00, 0xBC, 0x61, 0x4E,
			// Reason length: 6, Reason: "foo"
			0x06, 0x66, 0x6F, 0x6F,
			// Reason: "bar", 1 byte of padding
			0x62, 0x61, 0x72, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::ByePacket> packet{ RTC::NEW_RTCP::ByePacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 'r');

		// No space to add more ssrcs.
		REQUIRE_THROWS_AS(packet->AddSsrc(3333), MediaSoupTypeError);
		// No space for a reason larger than the current one.
		REQUIRE_THROWS_AS(packet->SetReason("lolololo"), MediaSoupTypeError);

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 'r');

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 'r');

		/* Modify it. */

		packet->AddSsrc(3333);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 'r');

		packet->SetReason("");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678, 3333 });
		REQUIRE(packet->HasReason() == false);
		REQUIRE(packet->GetReason() == "");

		packet->SetReason("abcde");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "abcde");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == 'e');
	}

	SECTION("ByePacket::Factory() succeeds")
	{
		std::unique_ptr<RTC::NEW_RTCP::ByePacket> packet{ RTC::NEW_RTCP::ByePacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		// TODO
	}
}
