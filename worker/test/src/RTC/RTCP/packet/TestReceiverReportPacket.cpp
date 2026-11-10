#include "common.hpp"
#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP Receiver Report Packet (201)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("ReceiverReportPacket::Parse() succeeds")
	{
		const auto checkReportBlocks = [](const RTC::RTCP::ReceiverReportPacket* packet) -> void
		{
			const auto reportBlocks = packet->GetReportBlocks();

			REQUIRE(packet->GetSsrc() == 0x5D931534);
			REQUIRE(packet->GetNumberOfReportBlocks() == 2);
			REQUIRE(reportBlocks.size() == 2);

			REQUIRE(reportBlocks[0].ssrc == 0x01932DB4);
			REQUIRE(reportBlocks[0].fractionLost == 80);
			REQUIRE(reportBlocks[0].totalLost == 8);
			REQUIRE(reportBlocks[0].extendedHighestSeq == 65541);
			REQUIRE(reportBlocks[0].jitter == 32);
			REQUIRE(reportBlocks[0].lastSenderReport == 0x12345678);
			REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 65536);

			REQUIRE(reportBlocks[1].ssrc == 1234);
			REQUIRE(reportBlocks[1].fractionLost == 0);
			REQUIRE(reportBlocks[1].totalLost == -1);
			REQUIRE(reportBlocks[1].extendedHighestSeq == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].jitter == 0);
			REQUIRE(reportBlocks[1].lastSenderReport == 0);
			REQUIRE(reportBlocks[1].delaySinceLastSenderReport == 0);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, RC=2, PT:201, Length: 14
			0b10000010, 0xC9, 0x00, 0x0E,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// Report block 1
			// SSRC: 0x01932DB4
			0x01, 0x93, 0x2D, 0xB4,
			// Fraction lost: 80, Total lost: 8
			0x50, 0x00, 0x00, 0x08,
			// Extended highest seq: 65541
			0x00, 0x01, 0x00, 0x05,
			// Jitter: 32
			0x00, 0x00, 0x00, 0x20,
			// LSR: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// DLSR: 65536
			0x00, 0x01, 0x00, 0x00,
			// Report block 2
			// SSRC: 1234
			0x00, 0x00, 0x04, 0xD2,
			// Fraction lost: 0, Total lost: -1
			0x00, 0xFF, 0xFF, 0xFF,
			// Extended highest seq: 4294967295
			0xFF, 0xFF, 0xFF, 0xFF,
			// Jitter: 0
			0x00, 0x00, 0x00, 0x00,
			// LSR: 0
			0x00, 0x00, 0x00, 0x00,
			// DLSR: 0
			0x00, 0x00, 0x00, 0x00,
			// Profile-specific extensions
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());

		// Cannot add report blocks to a packet with profile-specific extensions.
		REQUIRE_THROWS_AS(
		  packet->AddReportBlock(
		    { .ssrc                       = 3333,
				  .fractionLost               = 0,
				  .totalLost                  = 0,
				  .extendedHighestSeq         = 0,
				  .jitter                     = 0,
				  .lastSenderReport           = 0,
				  .delaySinceLastSenderReport = 0 }),
		  MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 60);

		checkReportBlocks(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());
	}

	SECTION("ReceiverReportPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, RC=0, PT:201, Length: 2
			0b10100000, 0xC9, 0x00, 0x02,
			// SSRC of packet sender: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04,
			// Extra space in the buffer
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSsrc() == 0x5D931534);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);

		/* Modify it (it removes the padding). */

		// Padding is not a profile-specific extension, so AddReportBlock() does
		// not throw.
		packet->AddReportBlock(
		  { .ssrc                       = 3333,
			  .fractionLost               = 1,
			  .totalLost                  = 2,
			  .extendedHighestSeq         = 3,
			  .jitter                     = 4,
			  .lastSenderReport           = 5,
			  .delaySinceLastSenderReport = 6 });

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		const auto reportBlocks = packet->GetReportBlocks();

		REQUIRE(packet->GetNumberOfReportBlocks() == 1);
		REQUIRE(reportBlocks.size() == 1);
		REQUIRE(reportBlocks[0].ssrc == 3333);
		REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 6);
	}

	SECTION("ReceiverReportPacket::Parse() fails")
	{
		// No space for the SSRC of packet sender.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, RC=0, PT:201, Length: 0
			0b10000000, 0xC9, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::ReceiverReportPacket::Parse(buffer1, sizeof(buffer1)));

		// RC field announces 1 report block but there is none.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, RC=1, PT:201, Length: 1
			0b10000001, 0xC9, 0x00, 0x01,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::ReceiverReportPacket::Parse(buffer2, sizeof(buffer2)));

		// RC field announces 2 report blocks but there is only 1.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, RC=2, PT:201, Length: 7
			0b10000010, 0xC9, 0x00, 0x07,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// Report block 1
			0x00, 0x00, 0x08, 0xAE,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::ReceiverReportPacket::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("ReceiverReportPacket::Factory() succeeds")
	{
		const auto checkReportBlocks = [](const RTC::RTCP::ReceiverReportPacket* packet) -> void
		{
			const auto reportBlocks = packet->GetReportBlocks();

			REQUIRE(packet->GetSsrc() == 1111);
			REQUIRE(packet->GetNumberOfReportBlocks() == 2);
			REQUIRE(reportBlocks.size() == 2);

			REQUIRE(reportBlocks[0].ssrc == 2222);
			REQUIRE(reportBlocks[0].fractionLost == 1);
			REQUIRE(reportBlocks[0].totalLost == 2);
			REQUIRE(reportBlocks[0].extendedHighestSeq == 3);
			REQUIRE(reportBlocks[0].jitter == 4);
			REQUIRE(reportBlocks[0].lastSenderReport == 5);
			REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 6);

			REQUIRE(reportBlocks[1].ssrc == 3333);
			REQUIRE(reportBlocks[1].fractionLost == 255);
			REQUIRE(reportBlocks[1].totalLost == -2);
			REQUIRE(reportBlocks[1].extendedHighestSeq == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].jitter == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].lastSenderReport == 0xFFFFFFFF);
			REQUIRE(reportBlocks[1].delaySinceLastSenderReport == 0xFFFFFFFF);
		};

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrc() == 0);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);
		REQUIRE(packet->GetReportBlocks().empty());

		/* Modify it. */

		packet->SetSsrc(1111);
		packet->AddReportBlock(
		  { .ssrc                       = 2222,
			  .fractionLost               = 1,
			  .totalLost                  = 2,
			  .extendedHighestSeq         = 3,
			  .jitter                     = 4,
			  .lastSenderReport           = 5,
			  .delaySinceLastSenderReport = 6 });
		packet->AddReportBlock(
		  { .ssrc                       = 3333,
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
		  /*length*/ 56,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 56,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 56,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::RR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkReportBlocks(packet.get());
	}

	SECTION("ReceiverReportPacket::AddReportBlock() clamps the total lost")
	{
		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		packet->AddReportBlock(
		  { .ssrc                       = 1111,
			  .fractionLost               = 0,
			  .totalLost                  = 8388607,
			  .extendedHighestSeq         = 0,
			  .jitter                     = 0,
			  .lastSenderReport           = 0,
			  .delaySinceLastSenderReport = 0 });
		packet->AddReportBlock(
		  { .ssrc                       = 2222,
			  .fractionLost               = 0,
			  .totalLost                  = 8388608,
			  .extendedHighestSeq         = 0,
			  .jitter                     = 0,
			  .lastSenderReport           = 0,
			  .delaySinceLastSenderReport = 0 });
		packet->AddReportBlock(
		  { .ssrc                       = 3333,
			  .fractionLost               = 0,
			  .totalLost                  = -8388608,
			  .extendedHighestSeq         = 0,
			  .jitter                     = 0,
			  .lastSenderReport           = 0,
			  .delaySinceLastSenderReport = 0 });
		packet->AddReportBlock(
		  { .ssrc                       = 4444,
			  .fractionLost               = 0,
			  .totalLost                  = -8388609,
			  .extendedHighestSeq         = 0,
			  .jitter                     = 0,
			  .lastSenderReport           = 0,
			  .delaySinceLastSenderReport = 0 });

		const auto reportBlocks = packet->GetReportBlocks();

		REQUIRE(reportBlocks.size() == 4);
		REQUIRE(reportBlocks[0].totalLost == 8388607);
		REQUIRE(reportBlocks[1].totalLost == 8388607);
		REQUIRE(reportBlocks[2].totalLost == -8388608);
		REQUIRE(reportBlocks[3].totalLost == -8388608);
	}

	SECTION("ReceiverReportPacket::AddReportBlock() throws")
	{
		const RTC::RTCP::AnyReportPacket::ReportBlock reportBlock{ .ssrc               = 1111,
		                                                               .fractionLost       = 0,
		                                                               .totalLost          = 0,
		                                                               .extendedHighestSeq = 0,
		                                                               .jitter             = 0,
		                                                               .lastSenderReport   = 0,
		                                                               .delaySinceLastSenderReport = 0 };

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		for (size_t idx{ 0 }; idx < RTC::RTCP::AnyReportPacket::MaxReportBlocks; ++idx)
		{
			packet->AddReportBlock(reportBlock);
		}

		REQUIRE(packet->GetNumberOfReportBlocks() == 31);
		REQUIRE(packet->GetLength() == 8 + (31 * 24));

		// No more than 31 report blocks.
		REQUIRE_THROWS_AS(packet->AddReportBlock(reportBlock), MediaSoupTypeError);

		REQUIRE(packet->GetNumberOfReportBlocks() == 31);
		REQUIRE(packet->GetLength() == 8 + (31 * 24));
	}

	SECTION("ReceiverReportPacket::AddReportBlock() throws if there is no space in the buffer")
	{
		const RTC::RTCP::AnyReportPacket::ReportBlock reportBlock{ .ssrc               = 1111,
		                                                               .fractionLost       = 0,
		                                                               .totalLost          = 0,
		                                                               .extendedHighestSeq = 0,
		                                                               .jitter             = 0,
		                                                               .lastSenderReport   = 0,
		                                                               .delaySinceLastSenderReport = 0 };

		std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer, 31)
		};

		REQUIRE(packet->GetLength() == 8);

		// The report block would need 32 bytes in total.
		REQUIRE_THROWS_AS(packet->AddReportBlock(reportBlock), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 8);
		REQUIRE(packet->GetNumberOfReportBlocks() == 0);
		REQUIRE(packet->GetReportBlocks().empty());
	}

	SECTION("ReceiverReportPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::ReceiverReportPacket::Factory(rtcpCommon::FactoryBuffer, 7), MediaSoupTypeError);
	}

	SECTION("ReceiverReportPacket::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::RTCP::ReceiverReportPacket::ComputeLengthFor(0) == 8);

		const std::unique_ptr<RTC::RTCP::ReceiverReportPacket> packet{
			RTC::RTCP::ReceiverReportPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		packet->AddReportBlock({ .ssrc = 7777 });
		packet->AddReportBlock({ .ssrc = 8888 });

		REQUIRE(RTC::RTCP::ReceiverReportPacket::ComputeLengthFor(2) == 56);
		REQUIRE(packet->GetLength() == RTC::RTCP::ReceiverReportPacket::ComputeLengthFor(2));
	}
}
