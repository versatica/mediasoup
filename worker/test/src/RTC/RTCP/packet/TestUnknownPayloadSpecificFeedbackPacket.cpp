#include "common.hpp"
#include "RTC/RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Unknown Payload-Specific Feedback Packet (206)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("UnknownPayloadSpecificFeedbackPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::UnknownPayloadSpecificFeedbackPacket* packet) -> void
		{
			REQUIRE(
			  packet->GetMessageType() ==
			  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
			  static_cast<RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType>(2));
			REQUIRE(packet->HasUnknownMessageType() == true);
			REQUIRE(packet->GetSenderSsrc() == 0x5D931534);
			REQUIRE(packet->GetMediaSsrc() == 0x01932DB4);
			REQUIRE(packet->HasUnknownValue() == true);
			REQUIRE(packet->GetUnknownValueLength() == 8);
			REQUIRE(packet->GetUnknownValue()[0] == 0x11);
			REQUIRE(packet->GetUnknownValue()[1] == 0x22);
			REQUIRE(packet->GetUnknownValue()[7] == 0x88);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=2, PT:206, Length: 4
			0b10000010, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// SSRC of media source: 0x01932DB4
			0x01, 0x93, 0x2D, 0xB4,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket> packet{
			RTC::RTCP::UnknownPayloadSpecificFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
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
		  /*length*/ 20,
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
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("UnknownPayloadSpecificFeedbackPacket::Parse() with an AFB that is not REMB succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 3
			0b10001111, 0xCE, 0x00, 0x03,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// 4 bytes of value
			0x00, 0x00, 0x00, 0x01
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket> packet{
			RTC::RTCP::UnknownPayloadSpecificFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  packet->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB);
		REQUIRE(packet->HasUnknownMessageType() == true);
		REQUIRE(packet->GetSenderSsrc() == 0xFA17FA17);
		REQUIRE(packet->GetMediaSsrc() == 0);
		REQUIRE(packet->HasUnknownValue() == true);
		REQUIRE(packet->GetUnknownValueLength() == 4);
		REQUIRE(packet->GetUnknownValue()[3] == 0x01);
	}

	SECTION("UnknownPayloadSpecificFeedbackPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=2, PT:206, Length: 3
			0b10100010, 0xCE, 0x00, 0x03,
			// Sender SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Media SSRC: 2222
			0x00, 0x00, 0x08, 0xAE,
			// 2 bytes of value, Padding: 2 bytes
			0xAA, 0xBB, 0x00, 0x02
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket> packet{
			RTC::RTCP::UnknownPayloadSpecificFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetMediaSsrc() == 2222);
		REQUIRE(packet->HasUnknownValue() == true);
		REQUIRE(packet->GetUnknownValueLength() == 2);
		REQUIRE(packet->GetUnknownValue()[0] == 0xAA);
		REQUIRE(packet->GetUnknownValue()[1] == 0xBB);
	}

	SECTION("UnknownPayloadSpecificFeedbackPacket::Parse() without value succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=2, PT:206, Length: 2
			0b10000010, 0xCE, 0x00, 0x02,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket> packet{
			RTC::RTCP::UnknownPayloadSpecificFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetMediaSsrc() == 2222);
		REQUIRE(packet->HasUnknownValue() == false);
		REQUIRE(packet->GetUnknownValue() == nullptr);
		REQUIRE(packet->GetUnknownValueLength() == 0);
	}

	SECTION("UnknownPayloadSpecificFeedbackPacket::Parse() fails")
	{
		// No space for the SSRC of media source.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=2, PT:206, Length: 1
			0b10000010, 0xCE, 0x00, 0x01,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::UnknownPayloadSpecificFeedbackPacket::Parse(buffer1, sizeof(buffer1)));
	}
}
