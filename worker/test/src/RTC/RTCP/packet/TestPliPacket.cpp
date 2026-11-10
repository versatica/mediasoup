#include "common.hpp"
#include "RTC/RTCP/packet/PliPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP PLI Packet (206, FMT 1)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("PliPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::PliPacket* packet) -> void
		{
			REQUIRE(
			  packet->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI);
			REQUIRE(packet->HasUnknownMessageType() == false);
			REQUIRE(packet->GetSenderSsrc() == 0x12345678);
			REQUIRE(packet->GetMediaSsrc() == 0x23456789);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 2
			0b10000001, 0xCE, 0x00, 0x02,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::PliPacket> packet{ RTC::RTCP::PliPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("PliPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=1, PT:206, Length: 3
			0b10100001, 0xCE, 0x00, 0x03,
			// Sender SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Media SSRC: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::PliPacket> packet{ RTC::RTCP::PliPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetMediaSsrc() == 2222);
	}

	SECTION("PliPacket::Parse() fails")
	{
		// No space for the SSRC of media source.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 1
			0b10000001, 0xCE, 0x00, 0x01,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::PliPacket::Parse(buffer1, sizeof(buffer1)));

		// Length field must be 2 since PLI has no Feedback Control Information
		// (FCI).
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 3
			0b10000001, 0xCE, 0x00, 0x03,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// 4 extra bytes
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::PliPacket::Parse(buffer2, sizeof(buffer2)));

		// Wrong message type (FMT 4, FIR).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, FMT=4, PT:206, Length: 2
			0b10000100, 0xCE, 0x00, 0x02,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::PliPacket::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("PliPacket::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expected[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 2
			0b10000001, 0xCE, 0x00, 0x02,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		// Dirty the buffer to check that Factory() writes zeros in the feedback
		// header.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::RTCP::PliPacket> packet{ RTC::RTCP::PliPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  packet->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI);
		REQUIRE(packet->GetSenderSsrc() == 0);
		REQUIRE(packet->GetMediaSsrc() == 0);

		/* Modify it. */

		packet->SetSenderSsrc(0x12345678);
		packet->SetMediaSsrc(0x23456789);

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expected, sizeof(expected)) == true);
	}

	SECTION("PliPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::PliPacket::Factory(rtcpCommon::FactoryBuffer, 11), MediaSoupTypeError);
	}
}
