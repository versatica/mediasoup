#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RTCP/packet/ByePacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("RTCP Bye Packet (203)", "[serializable][rtcp][packet]")
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

		std::unique_ptr<RTC::RTCP::ByePacket> packet{ RTC::RTCP::ByePacket::Parse(buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

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
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

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
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

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
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

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
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678, 3333 });
		REQUIRE(packet->HasReason() == false);
		REQUIRE(packet->GetReason().empty());

		packet->SetReason("abcde");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "abcde");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == 'e');
	}

	SECTION("ByePacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, SC=1, PT:203, Length: 4
			0b10100001, 0xCB, 0x00, 0x04,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Reason length: 3, Reason: "abc"
			0x03, 0x61, 0x62, 0x63,
			// Padding: 8 bytes
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x08
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::ByePacket> packet{ RTC::RTCP::ByePacket::Parse(buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 8);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "abc");

		/* Modify it (it removes the padding). */

		packet->AddSsrc(2222);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111, 2222 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "abc");
	}

	SECTION("ByePacket::Parse() with padding and SetReason() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, SC=1, PT:203, Length: 3
			0b10100001, 0xCB, 0x00, 0x03,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Padding: 8 bytes
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x08
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::ByePacket> packet{ RTC::RTCP::ByePacket::Parse(buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 8);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111 });
		REQUIRE(packet->HasReason() == false);

		/* Modify it (it removes the padding). */

		packet->SetReason("abc");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "abc");
	}

	SECTION("ByePacket::Parse() fails")
	{
		// Length field should be 5 (24 bytes).
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, SC=3, PT:203, Length: 6
			0b10000011, 0xCB, 0x00, 0x06,
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

		REQUIRE(!RTC::RTCP::ByePacket::Parse(buffer1, sizeof(buffer1)));

		// SC should be 3 instead of 4 (first byte should be 0b10000011). So the parser ends
		// reading a wrong value of the Reason length field and fails.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, SC=3, PT:203, Length: 5
			0b10000100, 0xCB, 0x00, 0x05,
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

		REQUIRE(!RTC::RTCP::ByePacket::Parse(buffer2, sizeof(buffer2)));

		// Parse() called with less buffer length than needed.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
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

		REQUIRE(!RTC::RTCP::ByePacket::Parse(buffer3, sizeof(buffer3) - 1));

		// Length field should be 2 (12 bytes) since the packet must end right
		// after the padded Reason, but it announces 4 extra bytes.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=0, SC=1, PT:203, Length: 3
			0b10000001, 0xCB, 0x00, 0x03,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Reason length: 1, Reason: "a", 2 bytes of padding
			0x01, 0x61, 0x00, 0x00,
			// 4 extra bytes
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::ByePacket::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("ByePacket::Factory() succeeds")
	{
		std::unique_ptr<RTC::RTCP::ByePacket> packet{ RTC::RTCP::ByePacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs().empty());
		REQUIRE(packet->HasReason() == false);
		REQUIRE(packet->GetReason().empty());

		/* Modify it. */

		packet->AddSsrc(1111);
		packet->AddSsrc(2222);
		// Reason string is 25 bytes, so padded(1 + 25) is needed = 28 bytes.
		packet->SetReason("amazing reason, isn't it?");
		packet->AddSsrc(3333);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4 + 4 + 4 + 4 + 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111, 2222, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "amazing reason, isn't it?");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == '?');

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 4 + 4 + 4 + 4 + 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111, 2222, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "amazing reason, isn't it?");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == '?');

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 4 + 4 + 4 + 4 + 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 1111, 2222, 3333 });
		REQUIRE(packet->HasReason() == true);
		REQUIRE(packet->GetReason() == "amazing reason, isn't it?");
		// Check padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == '?');
	}
}
