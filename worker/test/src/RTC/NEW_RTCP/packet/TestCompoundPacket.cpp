#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/ByePacket.hpp"
#include "RTC/NEW_RTCP/packet/CompoundPacket.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("RTCP Compound Packet (203)", "[serializable][rtcp][compoundpacket]")
{
	rtcpCommon::ResetBuffers();

	SECTION("CompoundPacket::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Bye Packet
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
			0x62, 0x61, 0x72, 0x00,
			// Unknown Packet
			// V=2, P=0, SC=5, PT:222, Length: 2
			0b10000101, 0xDE, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::CompoundPacket> compoundPacket{
			RTC::NEW_RTCP::CompoundPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*packetsCount*/ 2);

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ByePacket>() != nullptr);
		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPacket>() != nullptr);

		const auto* packet1 =
		  reinterpret_cast<const RTC::NEW_RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ByePacket>() == packet1);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::NEW_RTCP::ByePacket*>(packet1),
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet1->HasReason() == true);
		REQUIRE(packet1->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 1] == 0);
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 2] == 'r');

		const auto* packet2 =
		  reinterpret_cast<const RTC::NEW_RTCP::UnknownPacket*>(compoundPacket->GetPacketAt(1));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPacket>() == packet2);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::NEW_RTCP::UnknownPacket*>(packet2),
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::NEW_RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true);

		REQUIRE(packet2->HasUnknownValue() == true);
		REQUIRE(packet2->GetUnknownValueLength() == 8);
		REQUIRE(packet2->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet2->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet2->GetUnknownValue()[7] == 0x88);
	}
}
