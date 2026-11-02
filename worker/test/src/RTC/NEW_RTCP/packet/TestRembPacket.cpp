#include "common.hpp"
#include "RTC/NEW_RTCP/packet/RembPacket.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("RTCP REMB Packet (206, FMT 15)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("RembPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::RembPacket* packet) -> void
		{
			REQUIRE(
			  packet->GetMessageType() == RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB);
			REQUIRE(packet->HasUnknownMessageType() == false);
			REQUIRE(packet->GetSenderSsrc() == 0xFA17FA17);
			REQUIRE(packet->GetMediaSsrc() == 0);
			REQUIRE(packet->GetBitrate() == 122754);
			REQUIRE(packet->GetNumberOfSsrcs() == 2);
			REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 0x02D03702, 0x04A76747 });
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 6
			0b10001111, 0xCE, 0x00, 0x06,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 2, BR Exp: 0, BR Mantissa: 122754
			0x02, 0x01, 0xDF, 0x82,
			// SSRC feedback: 0x02D03702
			0x02, 0xD0, 0x37, 0x02,
			// SSRC feedback: 0x04A76747
			0x04, 0xA7, 0x67, 0x47
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::RembPacket> packet{ RTC::NEW_RTCP::RembPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false);

		checkPacket(packet.get());

		// No space to add more SSRCs.
		REQUIRE_THROWS_AS(packet->AddSsrc(1111), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 28);

		checkPacket(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false);

		checkPacket(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false);

		checkPacket(packet.get());
	}

	SECTION("RembPacket::Parse() fails")
	{
		// No space for Num SSRC, BR Exp and BR Mantissa.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 3
			0b10001111, 0xCE, 0x00, 0x03,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer1, sizeof(buffer1)));

		// Unique identifier is not 'R' 'E' 'M' 'B'.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 4
			0b10001111, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'N' 'E' 'M' 'B'
			0x4E, 0x45, 0x4D, 0x42,
			// Num SSRC: 0, BR Exp: 1, BR Mantissa: 0x3FB93
			0x00, 0x07, 0xFB, 0x93
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer2, sizeof(buffer2)));

		// Bitrate does not fit in 64 bits.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 4
			0b10001111, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 0, BR Exp: 63, BR Mantissa: 0x3FB93
			0x00, 0xFF, 0xFB, 0x93
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer3, sizeof(buffer3)));

		// Bitrate (200 * 2^56) does not fit in a signed 64 bits integer.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 4
			0b10001111, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 0, BR Exp: 56, BR Mantissa: 200
			0x00, 0xE0, 0x00, 0xC8
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer4, sizeof(buffer4)));

		// Num SSRC field announces 2 SSRCs but there is only 1.
		// clang-format off
		alignas(4) uint8_t buffer5[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 5
			0b10001111, 0xCE, 0x00, 0x05,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 2, BR Exp: 1, BR Mantissa: 0x3FB93
			0x02, 0x07, 0xFB, 0x93,
			// SSRC feedback: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer5, sizeof(buffer5)));

		// Wrong message type (FMT 1, PLI).
		// clang-format off
		alignas(4) uint8_t buffer6[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 4
			0b10000001, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 0, BR Exp: 1, BR Mantissa: 0x3FB93
			0x00, 0x07, 0xFB, 0x93
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RembPacket::Parse(buffer6, sizeof(buffer6)));
	}

	SECTION("RembPacket::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expected[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 7
			0b10001111, 0xCE, 0x00, 0x07,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 3, BR Exp: 1, BR Mantissa: 0x3FB93
			0x03, 0x07, 0xFB, 0x93,
			// SSRC feedback: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// SSRC feedback: 0x2345678A
			0x23, 0x45, 0x67, 0x8A,
			// SSRC feedback: 0x2345678B
			0x23, 0x45, 0x67, 0x8B
		};
		// clang-format on

		// Dirty the buffer to check that Factory() writes zeros in the header.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::NEW_RTCP::RembPacket> packet{ RTC::NEW_RTCP::RembPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false);

		REQUIRE(
		  packet->GetMessageType() == RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB);
		REQUIRE(packet->GetSenderSsrc() == 0);
		REQUIRE(packet->GetMediaSsrc() == 0);
		REQUIRE(packet->GetBitrate() == 0);
		REQUIRE(packet->GetNumberOfSsrcs() == 0);
		REQUIRE(packet->GetSsrcs().empty());

		/* Modify it. */

		packet->SetSenderSsrc(0x12345678);
		packet->SetBitrate(522022);
		packet->AddSsrc(0x23456789);
		packet->AddSsrc(0x2345678A);
		packet->AddSsrc(0x2345678B);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false);

		REQUIRE(packet->GetBitrate() == 522022);
		REQUIRE(packet->GetSsrcs() == std::vector<uint32_t>{ 0x23456789, 0x2345678A, 0x2345678B });
		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expected, sizeof(expected)) == true);

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expected, sizeof(expected)) == true);

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expected, sizeof(expected)) == true);
	}

	SECTION("RembPacket bitrate survives a round trip")
	{
		const auto checkBitrate = [](int64_t bitrate) -> void
		{
			std::unique_ptr<RTC::NEW_RTCP::RembPacket> packet{ RTC::NEW_RTCP::RembPacket::Factory(
				rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

			packet->SetBitrate(bitrate);

			const std::unique_ptr<RTC::NEW_RTCP::RembPacket> parsedPacket{ RTC::NEW_RTCP::RembPacket::Parse(
				packet->GetBuffer(), packet->GetLength()) };

			REQUIRE(parsedPacket);
			REQUIRE(parsedPacket->GetBitrate() == bitrate);
			REQUIRE(parsedPacket->GetSsrcs().empty());
		};

		checkBitrate(0);
		// Exponent 0 with the 2 high bits of the mantissa in use.
		checkBitrate(122754);
		checkBitrate(522022);
		// A bitrate that needs more than 32 bits.
		checkBitrate(int64_t{ 0x3FB93 } << 30);
	}

	SECTION("RembPacket::AddSsrc() and RembPacket::SetBitrate() throw")
	{
		std::unique_ptr<RTC::NEW_RTCP::RembPacket> packet{ RTC::NEW_RTCP::RembPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		for (uint32_t ssrc{ 1 }; ssrc <= RTC::NEW_RTCP::RembPacket::MaxSsrcs; ++ssrc)
		{
			packet->AddSsrc(ssrc);
		}

		REQUIRE(packet->GetNumberOfSsrcs() == 255);
		REQUIRE(packet->GetLength() == 20 + (255 * 4));

		// No more than 255 SSRCs.
		REQUIRE_THROWS_AS(packet->AddSsrc(256), MediaSoupTypeError);

		REQUIRE(packet->GetNumberOfSsrcs() == 255);
		REQUIRE(packet->GetLength() == 20 + (255 * 4));

		// Negative bitrate.
		REQUIRE_THROWS_AS(packet->SetBitrate(-1), MediaSoupTypeError);
	}

	SECTION("RembPacket::AddSsrc() throws if there is no space in the buffer")
	{
		std::unique_ptr<RTC::NEW_RTCP::RembPacket> packet{ RTC::NEW_RTCP::RembPacket::Factory(
			rtcpCommon::FactoryBuffer, 23) };

		REQUIRE(packet->GetLength() == 20);

		// The SSRC would need 24 bytes in total.
		REQUIRE_THROWS_AS(packet->AddSsrc(1111), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 20);
		REQUIRE(packet->GetNumberOfSsrcs() == 0);
		REQUIRE(packet->GetSsrcs().empty());
	}

	SECTION("RembPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::RembPacket::Factory(rtcpCommon::FactoryBuffer, 19), MediaSoupTypeError);
	}
}
