#include "common.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Unknown Packet", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("UnknownPacket::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, SC=5, PT:222, Length: 2
			0b10000101, 0xDE, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::UnknownPacket> packet{ RTC::NEW_RTCP::UnknownPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::NEW_RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true);

		REQUIRE(packet->HasUnknownValue() == true);
		REQUIRE(packet->GetUnknownValueLength() == 8);
		REQUIRE(packet->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet->GetUnknownValue()[7] == 0x88);

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::NEW_RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true);

		REQUIRE(packet->HasUnknownValue() == true);
		REQUIRE(packet->GetUnknownValueLength() == 8);
		REQUIRE(packet->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet->GetUnknownValue()[7] == 0x88);

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::NEW_RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true);

		REQUIRE(packet->HasUnknownValue() == true);
		REQUIRE(packet->GetUnknownValueLength() == 8);
		REQUIRE(packet->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet->GetUnknownValue()[7] == 0x88);
	}
}
