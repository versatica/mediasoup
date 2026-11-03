#include "common.hpp"
#include "RTC/NEW_RTCP/packet/NackPacket.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Generic NACK Packet (205, FMT 1)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("NackPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::NackPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(
			  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK);
			REQUIRE(packet->HasUnknownMessageType() == false);
			REQUIRE(packet->GetSenderSsrc() == 0x5D931534);
			REQUIRE(packet->GetMediaSsrc() == 0x01932DB4);
			REQUIRE(packet->GetNumberOfItems() == 2);
			REQUIRE(items.size() == 2);

			REQUIRE(items[0].packetId == 1000);
			REQUIRE(items[0].lostPacketBitmask == 0x8001);

			REQUIRE(items[1].packetId == 65535);
			REQUIRE(items[1].lostPacketBitmask == 0);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=1, PT:205, Length: 4
			0b10000001, 0xCD, 0x00, 0x04,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// SSRC of media source: 0x01932DB4
			0x01, 0x93, 0x2D, 0xB4,
			// PID: 1000, BLP: 0x8001
			0x03, 0xE8, 0x80, 0x01,
			// PID: 65535, BLP: 0x0000
			0xFF, 0xFF, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::NackPacket> packet{ RTC::NEW_RTCP::NackPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		// No space to add more items.
		REQUIRE_THROWS_AS(packet->AddItem(1, 0), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 20);

		checkPacket(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
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
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("NackPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=1, PT:205, Length: 4
			0b10100001, 0xCD, 0x00, 0x04,
			// Sender SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Media SSRC: 2222
			0x00, 0x00, 0x08, 0xAE,
			// PID: 100, BLP: 0x0001
			0x00, 0x64, 0x00, 0x01,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::NackPacket> packet{ RTC::NEW_RTCP::NackPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetMediaSsrc() == 2222);
		REQUIRE(packet->GetNumberOfItems() == 1);

		/* Modify it (it removes the padding). */

		packet->AddItem(200, 0x0002);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		const auto items = packet->GetItems();

		REQUIRE(packet->GetNumberOfItems() == 2);
		REQUIRE(items.size() == 2);
		REQUIRE(items[0].packetId == 100);
		REQUIRE(items[0].lostPacketBitmask == 0x0001);
		REQUIRE(items[1].packetId == 200);
		REQUIRE(items[1].lostPacketBitmask == 0x0002);
	}

	SECTION("NackPacket::Parse() fails")
	{
		// No space for the SSRC of media source.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=1, PT:205, Length: 1
			0b10000001, 0xCD, 0x00, 0x01,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::NackPacket::Parse(buffer1, sizeof(buffer1)));

		// No items.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, FMT=1, PT:205, Length: 2
			0b10000001, 0xCD, 0x00, 0x02,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::NackPacket::Parse(buffer2, sizeof(buffer2)));

		// Wrong message type (FMT 3, TMMBR).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, FMT=3, PT:205, Length: 3
			0b10000011, 0xCD, 0x00, 0x03,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// PID: 1000, BLP: 0x0000
			0x03, 0xE8, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::NackPacket::Parse(buffer3, sizeof(buffer3)));

		// The FCI (excluding padding) is not multiple of 4 bytes.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=1, FMT=1, PT:205, Length: 4
			0b10100001, 0xCD, 0x00, 0x04,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// PID: 1000, BLP: 0x0000
			0x03, 0xE8, 0x00, 0x00,
			// 2 extra bytes, Padding: 2 bytes
			0x11, 0x22, 0x00, 0x02
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::NackPacket::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("NackPacket::Factory() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::NackPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(
			  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK);
			REQUIRE(packet->GetSenderSsrc() == 1111);
			REQUIRE(packet->GetMediaSsrc() == 2222);
			REQUIRE(packet->GetNumberOfItems() == 2);
			REQUIRE(items.size() == 2);

			REQUIRE(items[0].packetId == 3333);
			REQUIRE(items[0].lostPacketBitmask == 0x0001);

			REQUIRE(items[1].packetId == 4444);
			REQUIRE(items[1].lostPacketBitmask == 0xFFFF);
		};

		// Dirty the buffer to check that Factory() writes zeros in the feedback
		// header.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::NEW_RTCP::NackPacket> packet{ RTC::NEW_RTCP::NackPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK);
		REQUIRE(packet->GetSenderSsrc() == 0);
		REQUIRE(packet->GetMediaSsrc() == 0);
		REQUIRE(packet->GetNumberOfItems() == 0);
		REQUIRE(packet->GetItems().empty());

		/* Modify it. */

		packet->SetSenderSsrc(1111);
		packet->SetMediaSsrc(2222);
		packet->AddItem(3333, 0x0001);
		packet->AddItem(4444, 0xFFFF);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
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
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("NackPacket::AddItem() throws if there is no space in the buffer")
	{
		std::unique_ptr<RTC::NEW_RTCP::NackPacket> packet{ RTC::NEW_RTCP::NackPacket::Factory(
			rtcpCommon::FactoryBuffer, 15) };

		REQUIRE(packet->GetLength() == 12);

		// The item would need 16 bytes in total.
		REQUIRE_THROWS_AS(packet->AddItem(1111, 0), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 12);
		REQUIRE(packet->GetNumberOfItems() == 0);
		REQUIRE(packet->GetItems().empty());
	}

	SECTION("NackPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::NackPacket::Factory(rtcpCommon::FactoryBuffer, 11), MediaSoupTypeError);
	}
}
