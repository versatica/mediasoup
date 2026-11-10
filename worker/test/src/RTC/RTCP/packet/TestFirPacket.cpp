#include "common.hpp"
#include "RTC/RTCP/packet/FirPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP FIR Packet (206, FMT 4)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("FirPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::FirPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(
			  packet->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR);
			REQUIRE(packet->HasUnknownMessageType() == false);
			REQUIRE(packet->GetSenderSsrc() == 0xFA17FA17);
			REQUIRE(packet->GetMediaSsrc() == 0);
			REQUIRE(packet->GetNumberOfItems() == 2);
			REQUIRE(items.size() == 2);

			REQUIRE(items[0].ssrc == 0x02D03702);
			REQUIRE(items[0].sequenceNumber == 4);

			REQUIRE(items[1].ssrc == 0x23456789);
			REQUIRE(items[1].sequenceNumber == 13);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=4, PT:206, Length: 6
			0b10000100, 0xCE, 0x00, 0x06,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC: 0x02D03702
			0x02, 0xD0, 0x37, 0x02,
			// Seq nr: 4, Reserved
			0x04, 0x00, 0x00, 0x00,
			// SSRC: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// Seq nr: 13, Reserved
			0x0D, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::FirPacket> packet{ RTC::RTCP::FirPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		// No space to add more items.
		REQUIRE_THROWS_AS(packet->AddItem(1111, 1), MediaSoupTypeError);

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
		  /*length*/ 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("FirPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=4, PT:206, Length: 5
			0b10100100, 0xCE, 0x00, 0x05,
			// Sender SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Media SSRC: 0
			0x00, 0x00, 0x00, 0x00,
			// Item 1
			// SSRC: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Seq nr.: 7, Reserved
			0x07, 0x00, 0x00, 0x00,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04,
			// Extra space in the buffer
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::FirPacket> packet{ RTC::RTCP::FirPacket::Parse(buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetNumberOfItems() == 1);

		/* Modify it (it removes the padding). */

		packet->AddItem(3333, 8);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		const auto items = packet->GetItems();

		REQUIRE(packet->GetNumberOfItems() == 2);
		REQUIRE(items.size() == 2);
		REQUIRE(items[0].ssrc == 2222);
		REQUIRE(items[0].sequenceNumber == 7);
		REQUIRE(items[1].ssrc == 3333);
		REQUIRE(items[1].sequenceNumber == 8);
	}

	SECTION("FirPacket::Parse() fails")
	{
		// No items.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=4, PT:206, Length: 2
			0b10000100, 0xCE, 0x00, 0x02,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::FirPacket::Parse(buffer1, sizeof(buffer1)));

		// One item and a half.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, FMT=4, PT:206, Length: 5
			0b10000100, 0xCE, 0x00, 0x05,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// Seq nr: 13, Reserved
			0x0D, 0x00, 0x00, 0x00,
			// Half item
			0x68, 0x61, 0x6C, 0x66
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::FirPacket::Parse(buffer2, sizeof(buffer2)));

		// Wrong message type (FMT 1, PLI).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, FMT=1, PT:206, Length: 4
			0b10000001, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// Seq nr: 13, Reserved
			0x0D, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::FirPacket::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("FirPacket::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expected[] =
		{
			// V=2, P=0, FMT=4, PT:206, Length: 4
			0b10000100, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC: 0x23456789
			0x23, 0x45, 0x67, 0x89,
			// Seq nr: 13, Reserved
			0x0D, 0x00, 0x00, 0x00
		};
		// clang-format on

		// Dirty the buffer to check that Factory() and AddItem() write zeros in
		// the feedback header and in the reserved bits.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::RTCP::FirPacket> packet{ RTC::RTCP::FirPacket::Factory(
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

		REQUIRE(packet->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR);
		REQUIRE(packet->GetSenderSsrc() == 0);
		REQUIRE(packet->GetMediaSsrc() == 0);
		REQUIRE(packet->GetNumberOfItems() == 0);
		REQUIRE(packet->GetItems().empty());

		/* Modify it. */

		packet->SetSenderSsrc(0x12345678);
		packet->AddItem(0x23456789, 13);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::PSFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

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

	SECTION("FirPacket::AddItem() throws if there is no space in the buffer")
	{
		std::unique_ptr<RTC::RTCP::FirPacket> packet{ RTC::RTCP::FirPacket::Factory(
			rtcpCommon::FactoryBuffer, 19) };

		REQUIRE(packet->GetLength() == 12);

		// The item would need 20 bytes in total.
		REQUIRE_THROWS_AS(packet->AddItem(1111, 1), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 12);
		REQUIRE(packet->GetNumberOfItems() == 0);
		REQUIRE(packet->GetItems().empty());
	}

	SECTION("FirPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::FirPacket::Factory(rtcpCommon::FactoryBuffer, 11), MediaSoupTypeError);
	}

	SECTION("FirPacket::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::RTCP::FirPacket::ComputeLengthFor(0) == 12);

		const std::unique_ptr<RTC::RTCP::FirPacket> packet{ RTC::RTCP::FirPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		packet->AddItem(1111, 1);
		packet->AddItem(2222, 2);

		REQUIRE(RTC::RTCP::FirPacket::ComputeLengthFor(2) == 28);
		REQUIRE(packet->GetLength() == RTC::RTCP::FirPacket::ComputeLengthFor(2));
	}
}
