#include "common.hpp"
#include "RTC/RTCP/packet/SenderReportPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Sender Report Packet (200)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("SenderReportPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::SenderReportPacket* packet) -> void
		{
			const auto reportBlocks = packet->GetReportBlocks();

			REQUIRE(packet->GetSsrc() == 0x5D931534);
			REQUIRE(packet->GetNtp().seconds == 0xE7A1B2C3);
			REQUIRE(packet->GetNtp().fractions == 0x12345678);
			REQUIRE(packet->GetRtpTimestamp() == 0x0A0B0C0D);
			REQUIRE(packet->GetPacketCount() == 1000);
			REQUIRE(packet->GetOctetCount() == 123456);
			REQUIRE(packet->GetNumberOfReportBlocks() == 1);
			REQUIRE(reportBlocks.size() == 1);

			REQUIRE(reportBlocks[0].ssrc == 0x01932DB4);
			REQUIRE(reportBlocks[0].fractionLost == 5);
			REQUIRE(reportBlocks[0].totalLost == 258);
			REQUIRE(reportBlocks[0].extendedHighestSeq == 131088);
			REQUIRE(reportBlocks[0].jitter == 15);
			REQUIRE(reportBlocks[0].lastSenderReport == 0xB2C31234);
			REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 32768);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, RC=1, PT:200, Length: 12
			0b10000001, 0xC8, 0x00, 0x0C,
			// SSRC of sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// NTP timestamp, most significant word: 0xE7A1B2C3
			0xE7, 0xA1, 0xB2, 0xC3,
			// NTP timestamp, least significant word: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// RTP timestamp: 0x0A0B0C0D
			0x0A, 0x0B, 0x0C, 0x0D,
			// Sender's packet count: 1000
			0x00, 0x00, 0x03, 0xE8,
			// Sender's octet count: 123456
			0x00, 0x01, 0xE2, 0x40,
			// Report block 1
			// SSRC: 0x01932DB4
			0x01, 0x93, 0x2D, 0xB4,
			// Fraction lost: 5, Total lost: 258
			0x05, 0x00, 0x01, 0x02,
			// Extended highest seq: 131088
			0x00, 0x02, 0x00, 0x10,
			// Jitter: 15
			0x00, 0x00, 0x00, 0x0F,
			// LSR: 0xB2C31234
			0xB2, 0xC3, 0x12, 0x34,
			// DLSR: 32768
			0x00, 0x00, 0x80, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{
			RTC::RTCP::SenderReportPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 52,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
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
		  /*length*/ 52,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
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
		  /*length*/ 52,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("SenderReportPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, RC=0, PT:200, Length: 7
			0b10100000, 0xC8, 0x00, 0x07,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// NTP timestamp (most significant word): 1111
			0x00, 0x00, 0x04, 0x57,
			// NTP timestamp (least significant word): 2222
			0x00, 0x00, 0x08, 0xAE,
			// RTP timestamp: 3333
			0x00, 0x00, 0x0D, 0x05,
			// Sender's packet count: 4444
			0x00, 0x00, 0x11, 0x5C,
			// Sender's octet count: 5555
			0x00, 0x00, 0x15, 0xB3,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{
			RTC::RTCP::SenderReportPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSsrc() == 0x5D931534);
		REQUIRE(packet->GetNtp().seconds == 1111);
		REQUIRE(packet->GetNtp().fractions == 2222);
		REQUIRE(packet->GetRtpTimestamp() == 3333);
		REQUIRE(packet->GetPacketCount() == 4444);
		REQUIRE(packet->GetOctetCount() == 5555);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);
		REQUIRE(packet->GetReportBlocks().empty());
	}

	SECTION("SenderReportPacket::Parse() fails")
	{
		// No space for the sender info.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, RC=0, PT:200, Length: 1
			0b10000000, 0xC8, 0x00, 0x01,
			// SSRC of sender: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SenderReportPacket::Parse(buffer1, sizeof(buffer1)));

		// RC field announces 1 report block but there is none.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, RC=1, PT:200, Length: 6
			0b10000001, 0xC8, 0x00, 0x06,
			// SSRC of sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// Sender info
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SenderReportPacket::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("SenderReportPacket::Factory() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::SenderReportPacket* packet) -> void
		{
			const auto reportBlocks = packet->GetReportBlocks();

			REQUIRE(packet->GetSsrc() == 1111);
			REQUIRE(packet->GetNtp().seconds == 2222);
			REQUIRE(packet->GetNtp().fractions == 3333);
			REQUIRE(packet->GetRtpTimestamp() == 4444);
			REQUIRE(packet->GetPacketCount() == 5555);
			REQUIRE(packet->GetOctetCount() == 6666);
			REQUIRE(packet->GetNumberOfReportBlocks() == 2);
			REQUIRE(reportBlocks.size() == 2);

			REQUIRE(reportBlocks[0].ssrc == 7777);
			REQUIRE(reportBlocks[0].fractionLost == 1);
			REQUIRE(reportBlocks[0].totalLost == 2);
			REQUIRE(reportBlocks[0].extendedHighestSeq == 3);
			REQUIRE(reportBlocks[0].jitter == 4);
			REQUIRE(reportBlocks[0].lastSenderReport == 5);
			REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 6);

			REQUIRE(reportBlocks[1].ssrc == 8888);
			REQUIRE(reportBlocks[1].fractionLost == 255);
			REQUIRE(reportBlocks[1].totalLost == -2);
			REQUIRE(reportBlocks[1].extendedHighestSeq == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].jitter == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].lastSenderReport == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].delaySinceLastSenderReport == 0xFFFFFFFF);
		};

		// Dirty the buffer to check that Factory() writes zeros in the fixed
		// header.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{ RTC::RTCP::SenderReportPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrc() == 0);
		REQUIRE(packet->GetNtp().seconds == 0);
		REQUIRE(packet->GetNtp().fractions == 0);
		REQUIRE(packet->GetRtpTimestamp() == 0);
		REQUIRE(packet->GetPacketCount() == 0);
		REQUIRE(packet->GetOctetCount() == 0);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);
		REQUIRE(packet->GetReportBlocks().empty());

		/* Modify it. */

		packet->SetSsrc(1111);
		packet->SetNtp({ .seconds = 2222, .fractions = 3333 });
		packet->SetRtpTimestamp(4444);
		packet->SetPacketCount(5555);
		packet->SetOctetCount(6666);
		packet->AddReportBlock(
		  { .ssrc                       = 7777,
			  .fractionLost               = 1,
			  .totalLost                  = 2,
			  .extendedHighestSeq         = 3,
			  .jitter                     = 4,
			  .lastSenderReport           = 5,
			  .delaySinceLastSenderReport = 6 });
		packet->AddReportBlock(
		  { .ssrc                       = 8888,
			  .fractionLost               = 255,
			  .totalLost                  = -2,
			  .extendedHighestSeq         = 0xFFFFFFFF,
			  .jitter                     = 0xFFFFFFFF,
			  .lastSenderReport           = 0xFFFFFFFF,
			  .delaySinceLastSenderReport = 0xFFFFFFFF });

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 76,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
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
		  /*length*/ 76,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
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
		  /*length*/ 76,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("SenderReportPacket::AddReportBlock() throws if there is no space in the buffer")
	{
		const RTC::RTCP::AnyReportPacket::ReportBlock reportBlock{ .ssrc               = 1111,
		                                                               .fractionLost       = 0,
		                                                               .totalLost          = 0,
		                                                               .extendedHighestSeq = 0,
		                                                               .jitter             = 0,
		                                                               .lastSenderReport   = 0,
		                                                               .delaySinceLastSenderReport = 0 };

		std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{
			RTC::RTCP::SenderReportPacket::Factory(rtcpCommon::FactoryBuffer, 51)
		};

		REQUIRE(packet->GetLength() == 28);

		// The report block would need 52 bytes in total.
		REQUIRE_THROWS_AS(packet->AddReportBlock(reportBlock), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 28);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);
		REQUIRE(packet->GetReportBlocks().empty());
	}

	SECTION("SenderReportPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::SenderReportPacket::Factory(rtcpCommon::FactoryBuffer, 27), MediaSoupTypeError);
	}

	SECTION("SenderReportPacket::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::RTCP::SenderReportPacket::ComputeLengthFor(0) == 28);

		const std::unique_ptr<RTC::RTCP::SenderReportPacket> packet{
			RTC::RTCP::SenderReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		packet->AddReportBlock({ .ssrc = 7777 });
		packet->AddReportBlock({ .ssrc = 8888 });

		REQUIRE(RTC::RTCP::SenderReportPacket::ComputeLengthFor(2) == 76);
		REQUIRE(packet->GetLength() == RTC::RTCP::SenderReportPacket::ComputeLengthFor(2));
	}
}
