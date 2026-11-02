#include "common.hpp"
#include "RTC/NEW_RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Unknown Transport Layer Feedback Packet (205)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("UnknownTransportLayerFeedbackPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket* packet) -> void
		{
			REQUIRE(
			  packet->GetMessageType() ==
			  RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::TMMBR);
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
			// V=2, P=0, FMT=3, PT:205, Length: 4
			0b10000011, 0xCD, 0x00, 0x04,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// SSRC of media source: 0x01932DB4
			0x01, 0x93, 0x2D, 0xB4,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket> packet{
			RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

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

	SECTION("UnknownTransportLayerFeedbackPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 3
			0b10101111, 0xCD, 0x00, 0x03,
			// Sender SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Media SSRC: 2222
			0x00, 0x00, 0x08, 0xAE,
			// 2 bytes of value, Padding: 2 bytes
			0xAA, 0xBB, 0x00, 0x02
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket> packet{
			RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
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

	SECTION("UnknownTransportLayerFeedbackPacket::Parse() without value succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=3, PT:205, Length: 2
			0b10000011, 0xCD, 0x00, 0x02,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket> packet{
			RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSenderSsrc() == 1111);
		REQUIRE(packet->GetMediaSsrc() == 2222);
		REQUIRE(packet->HasUnknownValue() == false);
		REQUIRE(packet->GetUnknownValue() == nullptr);
		REQUIRE(packet->GetUnknownValueLength() == 0);
	}

	SECTION("UnknownTransportLayerFeedbackPacket::Parse() fails")
	{
		// No space for the SSRC of media source.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=3, PT:205, Length: 1
			0b10000011, 0xCD, 0x00, 0x01,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket::Parse(buffer1, sizeof(buffer1)));
	}
}
