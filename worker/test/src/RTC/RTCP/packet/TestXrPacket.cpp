#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/XrPacket.hpp"
#include "RTC/RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/UnknownExtendedReport.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP XR Packet (207)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("XrPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::RTCP::XrPacket* packet) -> void
		{
			REQUIRE(packet->GetSsrc() == 0x5D931534);
			REQUIRE(packet->HasExtendedReports() == true);
			REQUIRE(packet->GetExtendedReportsCount() == 3);
			REQUIRE(packet->GetExtendedReportAt(3) == nullptr);

			const auto* const extendedReport1 =
			  static_cast<const RTC::RTCP::RrtExtendedReport*>(packet->GetExtendedReportAt(0));

			REQUIRE(
			  packet->GetFirstExtendedReportOfType<RTC::RTCP::RrtExtendedReport>() == extendedReport1);
			REQUIRE(extendedReport1->GetBuffer() == packet->GetBuffer() + 8);
			REQUIRE(extendedReport1->GetLength() == 12);
			REQUIRE(extendedReport1->GetType() == RTC::RTCP::ExtendedReport::BlockType::RRT);
			REQUIRE(extendedReport1->HasUnknownType() == false);
			REQUIRE(extendedReport1->GetNtp().seconds == 0xAABBCCDD);
			REQUIRE(extendedReport1->GetNtp().fractions == 0x11223344);

			const auto* const extendedReport2 =
			  static_cast<const RTC::RTCP::DlrrExtendedReport*>(packet->GetExtendedReportAt(1));

			REQUIRE(
			  packet->GetFirstExtendedReportOfType<RTC::RTCP::DlrrExtendedReport>() == extendedReport2);
			REQUIRE(extendedReport2->GetBuffer() == packet->GetBuffer() + 20);
			REQUIRE(extendedReport2->GetLength() == 16);
			REQUIRE(extendedReport2->GetType() == RTC::RTCP::ExtendedReport::BlockType::DLRR);
			REQUIRE(extendedReport2->HasUnknownType() == false);

			const auto subBlocks = extendedReport2->GetSubBlocks();

			REQUIRE(subBlocks.size() == 1);
			REQUIRE(subBlocks[0].ssrc == 0x11121314);
			REQUIRE(subBlocks[0].lastReceiverReport == 0x00110011);
			REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 0x11001100);

			const auto* const extendedReport3 =
			  static_cast<const RTC::RTCP::UnknownExtendedReport*>(packet->GetExtendedReportAt(2));

			REQUIRE(
			  packet->GetFirstExtendedReportOfType<RTC::RTCP::UnknownExtendedReport>() ==
			  extendedReport3);
			REQUIRE(extendedReport3->GetBuffer() == packet->GetBuffer() + 36);
			REQUIRE(extendedReport3->GetLength() == 8);
			// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
			REQUIRE(extendedReport3->GetType() == static_cast<RTC::RTCP::ExtendedReport::BlockType>(42));
			REQUIRE(extendedReport3->HasUnknownType() == true);
			REQUIRE(extendedReport3->GetUnknownValueLength() == 4);
			REQUIRE(extendedReport3->GetUnknownValue()[0] == 0x01);
			REQUIRE(extendedReport3->GetUnknownValue()[3] == 0x04);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 10
			0b10000000, 0xCF, 0x00, 0x0A,
			// SSRC: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// Extended Report block 1
			// BT: 4 (RRT), Type-specific: 0, Block length: 2
			0x04, 0x00, 0x00, 0x02,
			// NTP timestamp
			0xAA, 0xBB, 0xCC, 0xDD,
			0x11, 0x22, 0x33, 0x44,
			// Extended Report block 2
			// BT: 5 (DLRR), Type-specific: 0, Block length: 3
			0x05, 0x00, 0x00, 0x03,
			// SSRC: 0x11121314
			0x11, 0x12, 0x13, 0x14,
			// LRR: 0x00110011
			0x00, 0x11, 0x00, 0x11,
			// DLRR: 0x11001100
			0x11, 0x00, 0x11, 0x00,
			// Extended Report block 3
			// BT: 42, Type-specific: 0, Block length: 1
			0x2A, 0x00, 0x00, 0x01,
			// 4 bytes of value
			0x01, 0x02, 0x03, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 44,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
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
		  /*length*/ 44,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
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
		  /*length*/ 44,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("XrPacket::Parse() without Extended Report blocks succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 1
			0b10000000, 0xCF, 0x00, 0x01,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrc() == 0x12345678);
		REQUIRE(packet->HasExtendedReports() == false);
		REQUIRE(packet->GetExtendedReportsCount() == 0);
		REQUIRE(packet->GetExtendedReportAt(0) == nullptr);
		REQUIRE(packet->GetFirstExtendedReportOfType<RTC::RTCP::UnknownExtendedReport>() == nullptr);
	}

	SECTION("XrPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, Reserved=0, PT:207, Length: 4
			0b10100000, 0xCF, 0x00, 0x04,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// Extended Report block 1
			// BT: 42, Type-specific: 0, Block length: 1
			0x2A, 0x00, 0x00, 0x01,
			// 4 bytes of value
			0x01, 0x02, 0x03, 0x04,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04,
			// Extra space in the buffer
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		// clang-format off
		alignas(4) uint8_t extendedReportBuffer[] =
		{
			// BT: 99, Type-specific: 0, Block length: 0
			0x63, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSsrc() == 0x12345678);
		REQUIRE(packet->GetExtendedReportsCount() == 1);

		/* Modify it (it removes the padding). */

		const std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport{
			RTC::RTCP::UnknownExtendedReport::Parse(extendedReportBuffer, sizeof(extendedReportBuffer))
		};

		packet->AddExtendedReport(extendedReport.get());

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetExtendedReportsCount() == 2);

		const auto* const extendedReport2 = packet->GetExtendedReportAt(1);

		// The new Extended Report block goes where the padding was.
		REQUIRE(extendedReport2->GetBuffer() == buffer + 16);
		REQUIRE(extendedReport2->GetLength() == 4);
		// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		REQUIRE(extendedReport2->GetType() == static_cast<RTC::RTCP::ExtendedReport::BlockType>(99));
	}

	SECTION("XrPacket::Parse() with padding and a DLRR block succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, Reserved=0, PT:207, Length: 9
			0xA0, 0xCF, 0x00, 0x09,
			// SSRC: 0x5D931534
			0x5D, 0x93, 0x15, 0x34,
			// Extended Report block 1
			// BT: 5 (DLRR), Reserved: 0, Block length: 6
			0x05, 0x00, 0x00, 0x06,
			// SSRC 1: 0x11121314
			0x11, 0x12, 0x13, 0x14,
			// LRR 1: 0x00110011
			0x00, 0x11, 0x00, 0x11,
			// DLRR 1: 0x11001100
			0x11, 0x00, 0x11, 0x00,
			// SSRC 2: 0x21222324
			0x21, 0x22, 0x23, 0x24,
			// LRR 2: 0x00220022
			0x00, 0x22, 0x00, 0x22,
			// DLRR 2: 0x22002200
			0x22, 0x00, 0x22, 0x00,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 40,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetSsrc() == 0x5D931534);
		REQUIRE(packet->GetExtendedReportsCount() == 1);

		const auto* const extendedReport =
		  static_cast<const RTC::RTCP::DlrrExtendedReport*>(packet->GetExtendedReportAt(0));

		REQUIRE(extendedReport->GetType() == RTC::RTCP::ExtendedReport::BlockType::DLRR);
		REQUIRE(extendedReport->GetLength() == 28);

		const auto subBlocks = extendedReport->GetSubBlocks();

		REQUIRE(subBlocks.size() == 2);
		REQUIRE(subBlocks[0].ssrc == 0x11121314);
		REQUIRE(subBlocks[0].lastReceiverReport == 0x00110011);
		REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 0x11001100);
		REQUIRE(subBlocks[1].ssrc == 0x21222324);
		REQUIRE(subBlocks[1].lastReceiverReport == 0x00220022);
		REQUIRE(subBlocks[1].delaySinceLastReceiverReport == 0x22002200);
	}

	SECTION("XrPacket::Parse() fails")
	{
		// No space for the SSRC.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 0
			0b10000000, 0xCF, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer1, sizeof(buffer1)));

		// Extended Report block length field exceeds the packet length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 3
			0b10000000, 0xCF, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// BT: 42, Type-specific: 0, Block length: 2
			0x2A, 0x00, 0x00, 0x02,
			// 4 bytes of value
			0x01, 0x02, 0x03, 0x04
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer2, sizeof(buffer2)));

		// Bytes left after the last Extended Report block that don't form a
		// block (2 bytes followed by 2 bytes of padding).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=1, Reserved=0, PT:207, Length: 3
			0b10100000, 0xCF, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// BT: 42, Type-specific: 0, Block length: 0
			0x2A, 0x00, 0x00, 0x00,
			// 2 extra bytes, Padding: 2 bytes
			0x11, 0x22, 0x00, 0x02
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer3, sizeof(buffer3)));

		// Extended Report block overlapping the padding.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=1, Reserved=0, PT:207, Length: 3
			0b10100000, 0xCF, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// BT: 42, Type-specific: 0, Block length: 1
			0x2A, 0x00, 0x00, 0x01,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer4, sizeof(buffer4)));

		// Malformed RRT block (block length is not 2).
		// clang-format off
		alignas(4) uint8_t buffer5[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 3
			0b10000000, 0xCF, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// BT: 4 (RRT), Reserved: 0, Block length: 1
			0x04, 0x00, 0x00, 0x01,
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer5, sizeof(buffer5)));

		// Malformed DLRR block (sub-blocks length is not multiple of 12).
		// clang-format off
		alignas(4) uint8_t buffer6[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 3
			0b10000000, 0xCF, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// BT: 5 (DLRR), Reserved: 0, Block length: 1
			0x05, 0x00, 0x00, 0x01,
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer6, sizeof(buffer6)));

		// Wrong packet type (RR).
		// clang-format off
		alignas(4) uint8_t buffer7[] =
		{
			// V=2, P=0, RC=0, PT:201, Length: 1
			0b10000000, 0xC9, 0x00, 0x01,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::XrPacket::Parse(buffer7, sizeof(buffer7)));
	}

	SECTION("XrPacket::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t extendedReportBuffer1[] =
		{
			// BT: 42, Type-specific: 0xAB, Block length: 2
			0x2A, 0xAB, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		// clang-format off
		alignas(4) uint8_t extendedReportBuffer2[] =
		{
			// BT: 99, Type-specific: 0, Block length: 0
			0x63, 0x00, 0x00, 0x00
		};
		// clang-format on

		const auto checkPacket = [](const RTC::RTCP::XrPacket* packet) -> void
		{
			REQUIRE(packet->GetSsrc() == 1234);
			REQUIRE(packet->GetExtendedReportsCount() == 2);

			const auto* const extendedReport1 =
			  static_cast<const RTC::RTCP::UnknownExtendedReport*>(packet->GetExtendedReportAt(0));

			REQUIRE(extendedReport1->GetBuffer() == packet->GetBuffer() + 8);
			REQUIRE(extendedReport1->GetLength() == 12);
			// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
			REQUIRE(extendedReport1->GetType() == static_cast<RTC::RTCP::ExtendedReport::BlockType>(42));
			REQUIRE(extendedReport1->GetUnknownValueLength() == 8);
			REQUIRE(extendedReport1->GetUnknownValue()[0] == 0x11);
			REQUIRE(extendedReport1->GetUnknownValue()[7] == 0x88);

			const auto* const extendedReport2 =
			  static_cast<const RTC::RTCP::UnknownExtendedReport*>(packet->GetExtendedReportAt(1));

			REQUIRE(extendedReport2->GetBuffer() == packet->GetBuffer() + 20);
			REQUIRE(extendedReport2->GetLength() == 4);
			// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
			REQUIRE(extendedReport2->GetType() == static_cast<RTC::RTCP::ExtendedReport::BlockType>(99));
			REQUIRE(extendedReport2->HasUnknownValue() == false);
		};

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetSsrc() == 0);
		REQUIRE(packet->HasExtendedReports() == false);

		/* Modify it. */

		packet->SetSsrc(1234);

		const std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport1{
			RTC::RTCP::UnknownExtendedReport::Parse(extendedReportBuffer1, sizeof(extendedReportBuffer1))
		};
		const std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport2{
			RTC::RTCP::UnknownExtendedReport::Parse(extendedReportBuffer2, sizeof(extendedReportBuffer2))
		};

		packet->AddExtendedReport(extendedReport1.get());
		packet->AddExtendedReport(extendedReport2.get());

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Parse it. */

		const std::unique_ptr<RTC::RTCP::XrPacket> parsedPacket{ RTC::RTCP::XrPacket::Parse(
			packet->GetBuffer(), packet->GetLength()) };

		CHECK_RTCP_PACKET(
		  /*packet*/ parsedPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(parsedPacket.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
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
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("XrPacket::Factory() without Extended Report blocks succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// V=2, P=0, Reserved=0, PT:207, Length: 1
			0x80, 0xCF, 0x00, 0x01,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		packet->SetSsrc(0x12345678);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("XrPacket::BuildExtendedReportInPlace() succeeds")
	{
		constexpr size_t NumberOfSubBlocks{ 50 };

		const auto checkPacket = [](const RTC::RTCP::XrPacket* packet) -> void
		{
			REQUIRE(packet->GetSsrc() == 0x12345678);
			REQUIRE(packet->GetExtendedReportsCount() == 2);

			const auto* const rrtExtendedReport =
			  packet->GetFirstExtendedReportOfType<RTC::RTCP::RrtExtendedReport>();

			REQUIRE(rrtExtendedReport);
			REQUIRE(rrtExtendedReport == packet->GetExtendedReportAt(0));
			REQUIRE(rrtExtendedReport->GetBuffer() == packet->GetBuffer() + 8);
			REQUIRE(rrtExtendedReport->GetNtp().seconds == 1111);
			REQUIRE(rrtExtendedReport->GetNtp().fractions == 2222);

			const auto* const dlrrExtendedReport =
			  packet->GetFirstExtendedReportOfType<RTC::RTCP::DlrrExtendedReport>();

			REQUIRE(dlrrExtendedReport);
			REQUIRE(dlrrExtendedReport == packet->GetExtendedReportAt(1));
			REQUIRE(dlrrExtendedReport->GetBuffer() == packet->GetBuffer() + 20);
			REQUIRE(dlrrExtendedReport->GetLength() == 4 + (NumberOfSubBlocks * 12));

			const auto subBlocks = dlrrExtendedReport->GetSubBlocks();

			REQUIRE(subBlocks.size() == NumberOfSubBlocks);
			REQUIRE(subBlocks[0].ssrc == 1);
			REQUIRE(subBlocks[0].lastReceiverReport == 1001);
			REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 2001);
			REQUIRE(subBlocks[49].ssrc == 50);
			REQUIRE(subBlocks[49].lastReceiverReport == 1050);
			REQUIRE(subBlocks[49].delaySinceLastReceiverReport == 2050);
		};

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		packet->SetSsrc(0x12345678);

		// An Extended Report block built in place and discarded leaves the packet
		// as it was.
		auto* const discardedExtendedReport =
		  packet->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

		REQUIRE(packet->NeedsConsolidation() == true);

		discardedExtendedReport->Discard();

		REQUIRE(packet->NeedsConsolidation() == false);
		REQUIRE(packet->GetLength() == 8);
		REQUIRE(packet->HasExtendedReports() == false);

		auto* const rrtExtendedReport =
		  packet->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

		rrtExtendedReport->SetNtp({ .seconds = 1111, .fractions = 2222 });
		rrtExtendedReport->Consolidate();

		auto* const dlrrExtendedReport =
		  packet->BuildExtendedReportInPlace<RTC::RTCP::DlrrExtendedReport>();

		for (uint32_t idx{ 1 }; idx <= NumberOfSubBlocks; ++idx)
		{
			dlrrExtendedReport->AddSubBlock(
			  { .ssrc = idx, .lastReceiverReport = 1000 + idx, .delaySinceLastReceiverReport = 2000 + idx });
		}

		dlrrExtendedReport->Consolidate();

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 8 + 12 + 4 + (NumberOfSubBlocks * 12),
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Parse it. */

		const std::unique_ptr<RTC::RTCP::XrPacket> parsedPacket{ RTC::RTCP::XrPacket::Parse(
			packet->GetBuffer(), packet->GetLength()) };

		REQUIRE(parsedPacket);
		REQUIRE(parsedPacket->GetLength() == packet->GetLength());

		checkPacket(parsedPacket.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		checkPacket(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		checkPacket(packet.get());
	}

	SECTION("XrPacket::AddExtendedReport() throws if there is no space in the buffer")
	{
		// clang-format off
		alignas(4) uint8_t extendedReportBuffer[] =
		{
			// BT: 42, Type-specific: 0, Block length: 1
			0x2A, 0x00, 0x00, 0x01,
			// 4 bytes of value
			0x01, 0x02, 0x03, 0x04
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer, 12) };

		const std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport{
			RTC::RTCP::UnknownExtendedReport::Parse(extendedReportBuffer, sizeof(extendedReportBuffer))
		};

		REQUIRE_THROWS_AS(packet->AddExtendedReport(extendedReport.get()), MediaSoupTypeError);

		// The packet is left as it was.
		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 8,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::XR,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtendedReports() == false);
	}

	SECTION("XrPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::XrPacket::Factory(rtcpCommon::FactoryBuffer, 7), MediaSoupTypeError);
	}

	SECTION("XrPacket::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::RTCP::XrPacket::ComputeLengthFor(0) == 8);

		const std::unique_ptr<RTC::RTCP::XrPacket> packet{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		auto* const dlrrExtendedReport =
		  packet->BuildExtendedReportInPlace<RTC::RTCP::DlrrExtendedReport>();

		dlrrExtendedReport->AddSubBlock(
		  { .ssrc = 1, .lastReceiverReport = 1001, .delaySinceLastReceiverReport = 2001 });
		dlrrExtendedReport->AddSubBlock(
		  { .ssrc = 2, .lastReceiverReport = 1002, .delaySinceLastReceiverReport = 2002 });
		dlrrExtendedReport->Consolidate();

		const size_t length = RTC::RTCP::XrPacket::ComputeLengthFor(
		  RTC::RTCP::DlrrExtendedReport::ComputeLengthFor(2));

		REQUIRE(length == 36);
		REQUIRE(packet->GetLength() == length);
	}
}
