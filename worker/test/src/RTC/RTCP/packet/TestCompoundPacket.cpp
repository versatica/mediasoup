#include "common.hpp"
#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "RTC/RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "RTC/RTCP/packet/ByePacket.hpp"
#include "RTC/RTCP/packet/CompoundPacket.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/FirPacket.hpp"
#include "RTC/RTCP/packet/NackPacket.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "RTC/RTCP/packet/PliPacket.hpp"
#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/RTCP/packet/RembPacket.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "RTC/RTCP/packet/SenderReportPacket.hpp"
#include "RTC/RTCP/packet/TransportWideCcPacket.hpp"
#include "RTC/RTCP/packet/UnknownPacket.hpp"
#include "RTC/RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"
#include "RTC/RTCP/packet/XrPacket.hpp"
#include "RTC/RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("RTCP Compound Packet", "[serializable][rtcp][compoundpacket]")
{
	rtcpCommon::ResetBuffers();

	SECTION("CompoundPacket::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Bye Packet
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
			0x62, 0x61, 0x72, 0x00,
			// Unknown Packet
			// V=2, P=0, Count=5, PT:222, Length: 2
			0b10000101, 0xDE, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			// APP Packet (not implemented so it must be parsed as Unknown Packet)
			// V=2, P=0, Subtype=1, PT:204, Length: 3
			0b10000001, 0xCC, 0x00, 0x03,
			// SSRC: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Name: "abcd"
			0x61, 0x62, 0x63, 0x64,
			// 4 bytes of application-dependent data
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 52,
		  /*packetsCount*/ 3);

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::ByePacket>() != nullptr);
		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownPacket>() != nullptr);

		const auto* const packet1 =
		  static_cast<const RTC::RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::ByePacket>() == packet1);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::RTCP::ByePacket*>(packet1),
		  /*buffer*/ buffer,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet1->HasReason() == true);
		REQUIRE(packet1->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 1] == 0);
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 2] == 'r');

		const auto* const packet2 =
		  static_cast<const RTC::RTCP::UnknownPacket*>(compoundPacket->GetPacketAt(1));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownPacket>() == packet2);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::RTCP::UnknownPacket*>(packet2),
		  /*buffer*/ buffer + 24,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet2->HasUnknownValue() == true);
		REQUIRE(packet2->GetUnknownValueLength() == 8);
		REQUIRE(packet2->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet2->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet2->GetUnknownValue()[7] == 0x88);

		const auto* const packet3 =
		  static_cast<const RTC::RTCP::UnknownPacket*>(compoundPacket->GetPacketAt(2));

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::RTCP::UnknownPacket*>(packet3),
		  /*buffer*/ buffer + 36,
		  /*bufferLength*/ 16,
		  /*length*/ 16,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::RTCP::Packet::PacketType>(204),
		  /*unknownType*/ true,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet3->HasUnknownValue() == true);
		REQUIRE(packet3->GetUnknownValueLength() == 12);
		REQUIRE(packet3->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet3->GetUnknownValue()[11] == 0xDD);
	}

	SECTION("CompoundPacket::Parse() fails")
	{
		// A valid packet followed by a malformed BYE packet, whose SC field
		// announces 2 SSRCs that don't fit in its length, so the whole compound
		// packet is rejected.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Unknown Packet
			// V=2, P=0, Count=5, PT:222, Length: 2
			0b10000101, 0xDE, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			// Bye Packet
			// V=2, P=0, SC=2, PT:203, Length: 1
			0b10000010, 0xCB, 0x00, 0x01,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer1, sizeof(buffer1)));

		// A valid BYE packet followed by 2 bytes that don't form a packet.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Bye Packet
			// V=2, P=0, SC=1, PT:203, Length: 1
			0b10000001, 0xCB, 0x00, 0x01,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// 2 extra bytes
			0x11, 0x22
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer2, sizeof(buffer2)));

		// A BYE packet whose Length field announces more bytes than the remaining
		// ones.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Bye Packet
			// V=2, P=0, SC=1, PT:203, Length: 2
			0b10000001, 0xCB, 0x00, 0x02,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer3, sizeof(buffer3)));

		// The first byte is not RTCP (V=0).
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=0, P=0, SC=1, PT:203, Length: 1
			0b00000001, 0xCB, 0x00, 0x01,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer4, sizeof(buffer4)));

		// Less bytes than the RTCP Common Header.
		// clang-format off
		alignas(4) uint8_t buffer5[] =
		{
			0b10000001, 0xCB, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer5, sizeof(buffer5)));

		// The packet type is not in the RTCP range (192-223).
		// clang-format off
		alignas(4) uint8_t buffer6[] =
		{
			// V=2, P=0, Count=1, PT:0, Length: 1
			0b10000001, 0x00, 0x00, 0x01,
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer6, sizeof(buffer6)));

		// The Padding bit is set and the last byte announces more padding bytes
		// (87) than the packet has after the Common Header (4).
		// clang-format off
		alignas(4) uint8_t buffer7[] =
		{
			// Bye Packet
			// V=2, P=1, SC=1, PT:203, Length: 1
			0b10100001, 0xCB, 0x00, 0x01,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer7, sizeof(buffer7)));

		// The Padding bit is set but the last byte announces 0 padding bytes.
		// clang-format off
		alignas(4) uint8_t buffer8[] =
		{
			// Bye Packet
			// V=2, P=1, SC=1, PT:203, Length: 2
			0b10100001, 0xCB, 0x00, 0x02,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Padding: 4 bytes, the last one being 0
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer8, sizeof(buffer8)));

		// The Padding bit is set and the padding bytes (8) would eat the SSRC
		// that the SC field announces.
		// clang-format off
		alignas(4) uint8_t buffer9[] =
		{
			// Bye Packet
			// V=2, P=1, SC=1, PT:203, Length: 2
			0b10100001, 0xCB, 0x00, 0x02,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Padding: 4 bytes, the last one announcing 8
			0x00, 0x00, 0x00, 0x08
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer9, sizeof(buffer9)));
	}

	SECTION("CompoundPacket::Parse() succeeds with padding")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Bye Packet
			// V=2, P=1, SC=1, PT:203, Length: 2
			0b10100001, 0xCB, 0x00, 0x02,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04,
			// Unknown Packet
			// V=2, P=1, Count=5, PT:222, Length: 2
			0b10100101, 0xDE, 0x00, 0x02,
			// 5 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55,
			// Padding: 3 bytes
			0x00, 0x00, 0x03
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetsCount*/ 2);

		const auto* const packet1 =
		  static_cast<const RTC::RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::RTCP::ByePacket*>(packet1),
		  /*buffer*/ buffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111 });
		REQUIRE(packet1->HasReason() == false);

		const auto* const packet2 =
		  static_cast<const RTC::RTCP::UnknownPacket*>(compoundPacket->GetPacketAt(1));

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::RTCP::UnknownPacket*>(packet2),
		  /*buffer*/ buffer + 12,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		REQUIRE(packet2->HasUnknownValue() == true);
		REQUIRE(packet2->GetUnknownValueLength() == 5);
		REQUIRE(packet2->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet2->GetUnknownValue()[4] == 0x55);
	}

	SECTION("CompoundPacket::Factory() succeeds")
	{
		const auto checkPackets =
		  [](const RTC::RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));
			const auto* const packet2 =
			  static_cast<const RTC::RTCP::SdesPacket*>(compoundPacket->GetPacketAt(1));
			const auto sdesItems = packet2->GetItems();
			const auto* const packet3 =
			  static_cast<const RTC::RTCP::ReceiverReportPacket*>(compoundPacket->GetPacketAt(2));
			const auto reportBlocks = packet3->GetReportBlocks();
			const auto* const packet4 =
			  static_cast<const RTC::RTCP::SenderReportPacket*>(compoundPacket->GetPacketAt(3));
			const auto* const packet5 =
			  static_cast<const RTC::RTCP::NackPacket*>(compoundPacket->GetPacketAt(4));
			const auto nackItems = packet5->GetItems();
			const auto* const packet6 =
			  static_cast<const RTC::RTCP::PliPacket*>(compoundPacket->GetPacketAt(5));
			const auto* const packet7 =
			  static_cast<const RTC::RTCP::FirPacket*>(compoundPacket->GetPacketAt(6));
			const auto firItems = packet7->GetItems();
			const auto* const packet8 =
			  static_cast<const RTC::RTCP::RembPacket*>(compoundPacket->GetPacketAt(7));
			const auto* const packet9 =
			  static_cast<const RTC::RTCP::XrPacket*>(compoundPacket->GetPacketAt(8));
			const auto* const rrtExtendedReport =
			  static_cast<const RTC::RTCP::RrtExtendedReport*>(packet9->GetExtendedReportAt(0));
			const auto* const dlrrExtendedReport =
			  static_cast<const RTC::RTCP::DlrrExtendedReport*>(packet9->GetExtendedReportAt(1));
			const auto dlrrSubBlocks = dlrrExtendedReport->GetSubBlocks();

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 12);
			REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111 });
			REQUIRE(packet1->GetReason() == "foo");

			REQUIRE(packet2->GetBuffer() == buffer + 12);
			REQUIRE(packet2->GetLength() == 16);
			REQUIRE(packet2->GetNumberOfChunks() == 1);
			REQUIRE(sdesItems.size() == 1);
			REQUIRE(sdesItems[0].ssrc == 3333);
			REQUIRE(sdesItems[0].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
			REQUIRE(sdesItems[0].value == "abc");

			REQUIRE(packet3->GetBuffer() == buffer + 28);
			REQUIRE(packet3->GetLength() == 32);
			REQUIRE(packet3->GetSsrc() == 4444);
			REQUIRE(packet3->GetNumberOfReportBlocks() == 1);
			REQUIRE(reportBlocks.size() == 1);
			REQUIRE(reportBlocks[0].ssrc == 5555);
			REQUIRE(reportBlocks[0].fractionLost == 10);
			REQUIRE(reportBlocks[0].totalLost == -3);
			REQUIRE(reportBlocks[0].extendedHighestSeq == 100);
			REQUIRE(reportBlocks[0].jitter == 20);
			REQUIRE(reportBlocks[0].lastSenderReport == 30);
			REQUIRE(reportBlocks[0].delaySinceLastSenderReport == 40);

			REQUIRE(packet4->GetBuffer() == buffer + 60);
			REQUIRE(packet4->GetLength() == 28);
			REQUIRE(packet4->GetSsrc() == 6666);
			REQUIRE(packet4->GetNtp().seconds == 1);
			REQUIRE(packet4->GetNtp().fractions == 2);
			REQUIRE(packet4->GetRtpTimestamp() == 3);
			REQUIRE(packet4->GetPacketCount() == 4);
			REQUIRE(packet4->GetOctetCount() == 5);
			REQUIRE(packet4->GetNumberOfReportBlocks() == 0);
			REQUIRE(packet4->GetReportBlocks().empty());

			REQUIRE(packet5->GetBuffer() == buffer + 88);
			REQUIRE(packet5->GetLength() == 16);
			REQUIRE(
			  packet5->GetMessageType() == RTC::RTCP::AnyTransportLayerFeedbackPacket::MessageType::NACK);
			REQUIRE(packet5->GetSenderSsrc() == 7777);
			REQUIRE(packet5->GetMediaSsrc() == 8888);
			REQUIRE(packet5->GetNumberOfItems() == 1);
			REQUIRE(nackItems.size() == 1);
			REQUIRE(nackItems[0].packetId == 9999);
			REQUIRE(nackItems[0].lostPacketBitmask == 0x0005);

			REQUIRE(packet6->GetBuffer() == buffer + 104);
			REQUIRE(packet6->GetLength() == 12);
			REQUIRE(
			  packet6->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::PLI);
			REQUIRE(packet6->GetSenderSsrc() == 1001);
			REQUIRE(packet6->GetMediaSsrc() == 1002);

			REQUIRE(packet7->GetBuffer() == buffer + 116);
			REQUIRE(packet7->GetLength() == 20);
			REQUIRE(
			  packet7->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::FIR);
			REQUIRE(packet7->GetSenderSsrc() == 1003);
			REQUIRE(packet7->GetMediaSsrc() == 0);
			REQUIRE(packet7->GetNumberOfItems() == 1);
			REQUIRE(firItems.size() == 1);
			REQUIRE(firItems[0].ssrc == 1004);
			REQUIRE(firItems[0].sequenceNumber == 7);

			REQUIRE(packet8->GetBuffer() == buffer + 136);
			REQUIRE(packet8->GetLength() == 24);
			REQUIRE(
			  packet8->GetMessageType() == RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB);
			REQUIRE(packet8->GetSenderSsrc() == 1005);
			REQUIRE(packet8->GetMediaSsrc() == 0);
			REQUIRE(packet8->GetBitrate() == 522022);
			REQUIRE(packet8->GetSsrcs() == std::vector<uint32_t>{ 1006 });

			REQUIRE(packet9->GetBuffer() == buffer + 160);
			REQUIRE(packet9->GetLength() == 36);
			REQUIRE(packet9->GetSsrc() == 1007);
			REQUIRE(packet9->GetExtendedReportsCount() == 2);
			REQUIRE(rrtExtendedReport->GetBuffer() == buffer + 168);
			REQUIRE(rrtExtendedReport->GetType() == RTC::RTCP::ExtendedReport::BlockType::RRT);
			REQUIRE(rrtExtendedReport->GetNtp().seconds == 1008);
			REQUIRE(rrtExtendedReport->GetNtp().fractions == 1009);
			REQUIRE(dlrrExtendedReport->GetBuffer() == buffer + 180);
			REQUIRE(dlrrExtendedReport->GetType() == RTC::RTCP::ExtendedReport::BlockType::DLRR);
			REQUIRE(dlrrSubBlocks.size() == 1);
			REQUIRE(dlrrSubBlocks[0].ssrc == 1010);
			REQUIRE(dlrrSubBlocks[0].lastReceiverReport == 1011);
			REQUIRE(dlrrSubBlocks[0].delaySinceLastReceiverReport == 1012);
		};

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Factory(
			rtcpCommon::FactoryBuffer, 1000) };

		REQUIRE(compoundPacket->GetLength() == 0);
		REQUIRE(compoundPacket->GetPacketsCount() == 0);
		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		/* Clone it while empty. */

		const std::unique_ptr<RTC::RTCP::CompoundPacket> emptyClonedCompoundPacket{ compoundPacket->Clone(
			rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)) };

		REQUIRE(emptyClonedCompoundPacket->GetBuffer() == rtcpCommon::CloneBuffer);
		REQUIRE(emptyClonedCompoundPacket->GetLength() == 0);
		REQUIRE(emptyClonedCompoundPacket->GetPacketsCount() == 0);

		/* Build a packet and consolidate it. */

		auto* const packet1 = compoundPacket->BuildPacketInPlace<RTC::RTCP::ByePacket>();

		packet1->AddSsrc(1111);
		packet1->SetReason("foo");

		// The compound packet length doesn't include the packet being built in
		// place.
		REQUIRE(compoundPacket->GetLength() == 0);
		REQUIRE(compoundPacket->NeedsConsolidation() == true);
		REQUIRE(packet1->GetLength() == 12);

		packet1->Consolidate();

		REQUIRE(compoundPacket->GetLength() == 12);
		REQUIRE(compoundPacket->GetPacketsCount() == 1);
		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		/* Build a packet and discard it. */

		auto* const packet2 = compoundPacket->BuildPacketInPlace<RTC::RTCP::ByePacket>();

		packet2->AddSsrc(2222);
		packet2->AddSsrc(2223);

		REQUIRE(compoundPacket->GetLength() == 12);
		REQUIRE(packet2->GetLength() == 12);

		packet2->Discard();

		REQUIRE(compoundPacket->GetLength() == 12);
		REQUIRE(compoundPacket->GetPacketsCount() == 1);
		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		/* Build another packet in the same place and consolidate it. */

		auto* const packet3 = compoundPacket->BuildPacketInPlace<RTC::RTCP::SdesPacket>();

		REQUIRE(packet3->GetBuffer() == compoundPacket->GetBuffer() + 12);

		packet3->AddChunk(3333);
		packet3->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "abc");
		packet3->Consolidate();

		// Consolidate() or Discard() can only be called once.
		REQUIRE_THROWS_AS(packet3->Consolidate(), MediaSoupError);
		REQUIRE_THROWS_AS(packet3->Discard(), MediaSoupError);

		/* Build another packet and consolidate it. */

		auto* const packet4 = compoundPacket->BuildPacketInPlace<RTC::RTCP::ReceiverReportPacket>();

		REQUIRE(packet4->GetBuffer() == compoundPacket->GetBuffer() + 28);

		packet4->SetSsrc(4444);
		packet4->AddReportBlock(
		  { .ssrc                       = 5555,
			  .fractionLost               = 10,
			  .totalLost                  = -3,
			  .extendedHighestSeq         = 100,
			  .jitter                     = 20,
			  .lastSenderReport           = 30,
			  .delaySinceLastSenderReport = 40 });
		packet4->Consolidate();

		/* Build another packet and consolidate it. */

		auto* const packet5 = compoundPacket->BuildPacketInPlace<RTC::RTCP::SenderReportPacket>();

		REQUIRE(packet5->GetBuffer() == compoundPacket->GetBuffer() + 60);

		packet5->SetSsrc(6666);
		packet5->SetNtp({ .seconds = 1, .fractions = 2 });
		packet5->SetRtpTimestamp(3);
		packet5->SetPacketCount(4);
		packet5->SetOctetCount(5);
		packet5->Consolidate();

		/* Build another packet and consolidate it. */

		auto* const packet6 = compoundPacket->BuildPacketInPlace<RTC::RTCP::NackPacket>();

		REQUIRE(packet6->GetBuffer() == compoundPacket->GetBuffer() + 88);

		packet6->SetSenderSsrc(7777);
		packet6->SetMediaSsrc(8888);
		packet6->AddItem(9999, 0x0005);
		packet6->Consolidate();

		/* Build another packet and consolidate it. */

		auto* const packet7 = compoundPacket->BuildPacketInPlace<RTC::RTCP::PliPacket>();

		REQUIRE(packet7->GetBuffer() == compoundPacket->GetBuffer() + 104);

		packet7->SetSenderSsrc(1001);
		packet7->SetMediaSsrc(1002);
		packet7->Consolidate();

		/* Build another packet and consolidate it. */

		auto* const packet8 = compoundPacket->BuildPacketInPlace<RTC::RTCP::FirPacket>();

		REQUIRE(packet8->GetBuffer() == compoundPacket->GetBuffer() + 116);

		packet8->SetSenderSsrc(1003);
		packet8->AddItem(1004, 7);
		packet8->Consolidate();

		/* Build another packet and consolidate it. */

		auto* const packet9 = compoundPacket->BuildPacketInPlace<RTC::RTCP::RembPacket>();

		REQUIRE(packet9->GetBuffer() == compoundPacket->GetBuffer() + 136);

		packet9->SetSenderSsrc(1005);
		packet9->SetBitrate(522022);
		packet9->AddSsrc(1006);
		packet9->Consolidate();

		/* Build another packet, with Extended Report blocks built in place, and
		 * consolidate it. */

		auto* const packet10 = compoundPacket->BuildPacketInPlace<RTC::RTCP::XrPacket>();

		REQUIRE(packet10->GetBuffer() == compoundPacket->GetBuffer() + 160);

		packet10->SetSsrc(1007);

		auto* const rrtExtendedReport =
		  packet10->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

		rrtExtendedReport->SetNtp({ .seconds = 1008, .fractions = 1009 });
		rrtExtendedReport->Consolidate();

		auto* const dlrrExtendedReport =
		  packet10->BuildExtendedReportInPlace<RTC::RTCP::DlrrExtendedReport>();

		dlrrExtendedReport->AddSubBlock(
		  { .ssrc = 1010, .lastReceiverReport = 1011, .delaySinceLastReceiverReport = 1012 });
		dlrrExtendedReport->Consolidate();

		packet10->Consolidate();

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 1000,
		  /*length*/ 196,
		  /*packetsCount*/ 9);

		checkPackets(compoundPacket.get(), rtcpCommon::FactoryBuffer);

		/* Serialize it. */

		compoundPacket->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 196,
		  /*packetsCount*/ 9);

		checkPackets(compoundPacket.get(), rtcpCommon::SerializeBuffer);

		/* Clone it. */

		compoundPacket.reset(
		  compoundPacket->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 196,
		  /*packetsCount*/ 9);

		checkPackets(compoundPacket.get(), rtcpCommon::CloneBuffer);
	}

	SECTION("CompoundPacket::Parse() dispatches AFB packets by application")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// REMB Packet
			// V=2, P=0, FMT=15, PT:206, Length: 4
			0b10001111, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Unique identifier: 'R' 'E' 'M' 'B'
			0x52, 0x45, 0x4D, 0x42,
			// Num SSRC: 0, BR Exp: 1, BR Mantissa: 0x3FB93
			0x00, 0x07, 0xFB, 0x93,
			// AFB Packet that is not REMB
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

		const std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*packetsCount*/ 2);

		const auto* const rembPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::RembPacket>();
		const auto* const unknownPacket =
		  compoundPacket->GetFirstPacketOfType<RTC::RTCP::UnknownPayloadSpecificFeedbackPacket>();

		REQUIRE(rembPacket == compoundPacket->GetPacketAt(0));
		REQUIRE(rembPacket->GetBitrate() == 522022);
		REQUIRE(unknownPacket == compoundPacket->GetPacketAt(1));
		REQUIRE(
		  unknownPacket->GetMessageType() ==
		  RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType::AFB);
		REQUIRE(unknownPacket->HasUnknownMessageType() == true);
	}

	SECTION("CompoundPacket::Parse() with a XR packet, Serialize() and Clone() succeed")
	{
		const auto checkPackets =
		  [](const RTC::RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));
			const auto* const packet2 =
			  static_cast<const RTC::RTCP::XrPacket*>(compoundPacket->GetPacketAt(1));
			const auto* const rrtExtendedReport =
			  static_cast<const RTC::RTCP::RrtExtendedReport*>(packet2->GetExtendedReportAt(0));
			const auto* const dlrrExtendedReport =
			  static_cast<const RTC::RTCP::DlrrExtendedReport*>(packet2->GetExtendedReportAt(1));
			const auto dlrrSubBlocks = dlrrExtendedReport->GetSubBlocks();

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 8);
			REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111 });

			REQUIRE(packet2->GetBuffer() == buffer + 8);
			REQUIRE(packet2->GetLength() == 36);
			REQUIRE(packet2->GetSsrc() == 0x12345678);
			REQUIRE(packet2->GetExtendedReportsCount() == 2);

			REQUIRE(rrtExtendedReport->GetBuffer() == buffer + 16);
			REQUIRE(rrtExtendedReport->GetLength() == 12);
			REQUIRE(rrtExtendedReport->GetType() == RTC::RTCP::ExtendedReport::BlockType::RRT);
			REQUIRE(rrtExtendedReport->GetNtp().seconds == 1111);
			REQUIRE(rrtExtendedReport->GetNtp().fractions == 2222);

			REQUIRE(dlrrExtendedReport->GetBuffer() == buffer + 28);
			REQUIRE(dlrrExtendedReport->GetLength() == 16);
			REQUIRE(dlrrExtendedReport->GetType() == RTC::RTCP::ExtendedReport::BlockType::DLRR);
			REQUIRE(dlrrSubBlocks.size() == 1);
			REQUIRE(dlrrSubBlocks[0].ssrc == 3333);
			REQUIRE(dlrrSubBlocks[0].lastReceiverReport == 4444);
			REQUIRE(dlrrSubBlocks[0].delaySinceLastReceiverReport == 5555);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Bye Packet
			// V=2, P=0, SC=1, PT:203, Length: 1
			0b10000001, 0xCB, 0x00, 0x01,
			// SSRC 1: 1111
			0x00, 0x00, 0x04, 0x57,
			// XR Packet
			// V=2, P=0, Reserved=0, PT:207, Length: 8
			0b10000000, 0xCF, 0x00, 0x08,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// Extended Report block 1
			// BT: 4 (RRT), Reserved: 0, Block length: 2
			0x04, 0x00, 0x00, 0x02,
			// NTP timestamp, most significant word: 1111
			0x00, 0x00, 0x04, 0x57,
			// NTP timestamp, least significant word: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Extended Report block 2
			// BT: 5 (DLRR), Reserved: 0, Block length: 3
			0x05, 0x00, 0x00, 0x03,
			// SSRC: 3333
			0x00, 0x00, 0x0D, 0x05,
			// LRR: 4444
			0x00, 0x00, 0x11, 0x5C,
			// DLRR: 5555
			0x00, 0x00, 0x15, 0xB3
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 44,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), buffer);

		/* Serialize it. */

		compoundPacket->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 44,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), rtcpCommon::SerializeBuffer);

		/* Clone it. */

		compoundPacket.reset(
		  compoundPacket->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 44,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), rtcpCommon::CloneBuffer);
	}

	SECTION("CompoundPacket::Parse() with a Transport-wide CC packet, Serialize() and Clone() succeed")
	{
		const auto checkPackets =
		  [](const RTC::RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::RTCP::NackPacket*>(compoundPacket->GetPacketAt(0));
			const auto* const packet2 =
			  static_cast<const RTC::RTCP::TransportWideCcPacket*>(compoundPacket->GetPacketAt(1));
			const auto items              = packet1->GetItems();
			const auto receivedRtpPackets = packet2->GetReceivedRtpPackets();

			REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::TransportWideCcPacket>() == packet2);

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 16);
			REQUIRE(items.size() == 1);
			REQUIRE(items[0].packetId == 1000);

			REQUIRE(packet2->GetBuffer() == buffer + 16);
			REQUIRE(packet2->GetLength() == 24);
			REQUIRE(packet2->HasPadding() == true);
			REQUIRE(packet2->GetPaddingLength() == 1);
			REQUIRE(packet2->GetSenderSsrc() == 0xFA17FA17);
			REQUIRE(packet2->GetMediaSsrc() == 1234);
			REQUIRE(packet2->GetBaseSequenceNumber() == 13);
			REQUIRE(packet2->GetPacketStatusCount() == 1);
			REQUIRE(packet2->GetBaseTimeUs() == 1076014080000);
			REQUIRE(packet2->HasDeltas() == true);
			REQUIRE(receivedRtpPackets.size() == 1);
			REQUIRE(receivedRtpPackets[0].sequenceNumber == 13);
			REQUIRE(receivedRtpPackets[0].deltaTicks == 228);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// NACK Packet
			// V=2, P=0, FMT=1, PT:205, Length: 3
			0b10000001, 0xCD, 0x00, 0x03,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// PID: 1000, BLP: 0x0000
			0x03, 0xE8, 0x00, 0x00,
			// Transport-wide CC Packet
			// V=2, P=1, FMT=15, PT:205, Length: 5
			0xAF, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 1234
			0x00, 0x00, 0x04, 0xD2,
			// Base sequence number: 13, Packet status count: 1
			0x00, 0x0D, 0x00, 0x01,
			// Reference time: 35504, Feedback packet count: 0
			0x00, 0x8A, 0xB0, 0x00,
			// Run Length Chunk: symbol 1, run length 1
			0x20, 0x01,
			// Receive delta: 228
			0xE4,
			// Padding: 1 byte
			0x01
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 40,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), buffer);

		/* Serialize it. */

		compoundPacket->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 40,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), rtcpCommon::SerializeBuffer);

		/* Clone it. */

		compoundPacket.reset(
		  compoundPacket->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 40,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), rtcpCommon::CloneBuffer);
	}

	SECTION("CompoundPacket::Parse() with a malformed Transport-wide CC packet fails")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// NACK Packet
			// V=2, P=0, FMT=1, PT:205, Length: 3
			0b10000001, 0xCD, 0x00, 0x03,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// PID: 1000, BLP: 0x0000
			0x03, 0xE8, 0x00, 0x00,
			// Transport-wide CC Packet whose packet chunks do not cover the packet
			// status count
			// V=2, P=0, FMT=15, PT:205, Length: 5
			0x8F, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 20
			0x00, 0x01, 0x00, 0x14,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// One Bit Status Vector Chunk: 14 x received
			0xBF, 0xFF,
			// Run Length Chunk: symbol 0, run length 0
			0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::CompoundPacket::Parse(buffer, sizeof(buffer)));
	}

	SECTION("CompoundPacket::Factory() using AddPacket() with a Transport-wide CC packet succeeds")
	{
		const auto checkPackets =
		  [](const RTC::RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::RTCP::TransportWideCcPacket*>(compoundPacket->GetPacketAt(0));
			const auto receivedRtpPackets = packet1->GetReceivedRtpPackets();

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 28);
			REQUIRE(packet1->HasPadding() == true);
			REQUIRE(packet1->GetPaddingLength() == 3);
			REQUIRE(packet1->GetSenderSsrc() == 1111);
			REQUIRE(packet1->GetMediaSsrc() == 2222);
			REQUIRE(packet1->GetBaseSequenceNumber() == 1000);
			REQUIRE(packet1->GetPacketStatusCount() == 4);
			REQUIRE(packet1->GetBaseTimeUs() == 1000000000000);
			REQUIRE(packet1->GetFeedbackPacketCount() == 3);
			REQUIRE(receivedRtpPackets.size() == 3);
			REQUIRE(receivedRtpPackets[0].sequenceNumber == 1000);
			REQUIRE(receivedRtpPackets[0].deltaTicks == 4);
			REQUIRE(receivedRtpPackets[1].sequenceNumber == 1001);
			REQUIRE(receivedRtpPackets[1].deltaTicks == 4);
			REQUIRE(receivedRtpPackets[2].sequenceNumber == 1003);
			REQUIRE(receivedRtpPackets[2].deltaTicks == 4);
		};

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Factory(
			rtcpCommon::FactoryBuffer, 1000) };

		// Transport-wide CC packet, length: 28 bytes (3 of them padding).
		std::unique_ptr<RTC::RTCP::TransportWideCcPacket> packet1{ RTC::RTCP::TransportWideCcPacket::Factory(
			rtcpCommon::FactoryBuffer + 1000, 1000, /*includeDeltas*/ true) };

		packet1->SetSenderSsrc(1111);
		packet1->SetMediaSsrc(2222);
		packet1->SetFeedbackPacketCount(3);
		packet1->SetBase(1000, 1000000000000);

		REQUIRE(packet1->AddReceivedRtpPacket(1000, 1000000001000));
		REQUIRE(packet1->AddReceivedRtpPacket(1001, 1000000002000));
		REQUIRE(packet1->AddReceivedRtpPacket(1003, 1000000003000));

		compoundPacket->AddPacket(packet1.get());

		// NOTE: The stored packet is not the same than the given one since it is
		// internally cloned.
		REQUIRE(compoundPacket->GetPacketAt(0) != packet1.get());

		// Once added, we can delete the packet.
		packet1.reset();

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 1000,
		  /*length*/ 28,
		  /*packetsCount*/ 1);

		checkPackets(compoundPacket.get(), rtcpCommon::FactoryBuffer);

		/* Parse it. */

		const std::unique_ptr<RTC::RTCP::CompoundPacket> parsedCompoundPacket{
			RTC::RTCP::CompoundPacket::Parse(compoundPacket->GetBuffer(), compoundPacket->GetLength())
		};

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ parsedCompoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 28,
		  /*packetsCount*/ 1);

		checkPackets(parsedCompoundPacket.get(), rtcpCommon::FactoryBuffer);
	}

	SECTION("CompoundPacket::Factory() using AddPacket() succeeds")
	{
		const auto checkPackets =
		  [](const RTC::RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));
			const auto* const packet2 =
			  static_cast<const RTC::RTCP::XrPacket*>(compoundPacket->GetPacketAt(1));
			const auto* const rrtExtendedReport =
			  static_cast<const RTC::RTCP::RrtExtendedReport*>(packet2->GetExtendedReportAt(0));

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 12);
			REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111 });
			REQUIRE(packet1->GetReason() == "foo");

			REQUIRE(packet2->GetBuffer() == buffer + 12);
			REQUIRE(packet2->GetLength() == 20);
			REQUIRE(packet2->GetSsrc() == 2222);
			REQUIRE(packet2->GetExtendedReportsCount() == 1);
			REQUIRE(rrtExtendedReport->GetBuffer() == buffer + 20);
			REQUIRE(rrtExtendedReport->GetNtp().seconds == 3333);
			REQUIRE(rrtExtendedReport->GetNtp().fractions == 4444);
		};

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Factory(
			rtcpCommon::FactoryBuffer, 1000) };

		// BYE packet, length: 12 bytes.
		std::unique_ptr<RTC::RTCP::ByePacket> packet1{ RTC::RTCP::ByePacket::Factory(
			rtcpCommon::FactoryBuffer + 1000, 1000) };

		packet1->AddSsrc(1111);
		packet1->SetReason("foo");

		// XR packet with a RRT block, length: 20 bytes.
		std::unique_ptr<RTC::RTCP::XrPacket> packet2{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer + 2000, 1000) };

		packet2->SetSsrc(2222);

		auto* const rrtExtendedReport =
		  packet2->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

		rrtExtendedReport->SetNtp({ .seconds = 3333, .fractions = 4444 });
		rrtExtendedReport->Consolidate();

		compoundPacket->AddPacket(packet1.get());
		compoundPacket->AddPacket(packet2.get());

		// NOTE: The stored packets are not the same than the given ones since they
		// are internally cloned.
		REQUIRE(compoundPacket->GetPacketAt(0) != packet1.get());
		REQUIRE(compoundPacket->GetPacketAt(1) != packet2.get());

		// Once added, we can delete the packets.
		packet1.reset();
		packet2.reset();

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 1000,
		  /*length*/ 32,
		  /*packetsCount*/ 2);

		checkPackets(compoundPacket.get(), rtcpCommon::FactoryBuffer);

		/* Parse it. */

		const std::unique_ptr<RTC::RTCP::CompoundPacket> parsedCompoundPacket{
			RTC::RTCP::CompoundPacket::Parse(compoundPacket->GetBuffer(), compoundPacket->GetLength())
		};

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ parsedCompoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 32,
		  /*length*/ 32,
		  /*packetsCount*/ 2);

		checkPackets(parsedCompoundPacket.get(), rtcpCommon::FactoryBuffer);
	}

	SECTION("CompoundPacket::AddPacket() throws and leaves the CompoundPacket as it was")
	{
		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Factory(
			rtcpCommon::FactoryBuffer, 16) };

		// XR packet with a RRT block, length: 20 bytes, which exceeds the compound
		// packet buffer length.
		std::unique_ptr<RTC::RTCP::XrPacket> packet1{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer + 1000, 1000) };

		packet1->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>()->Consolidate();

		REQUIRE(packet1->GetLength() == 20);
		REQUIRE_THROWS_AS(compoundPacket->AddPacket(packet1.get()), MediaSoupTypeError);
		REQUIRE(compoundPacket->GetLength() == 0);
		REQUIRE(compoundPacket->GetPacketsCount() == 0);
		// Nothing was written into the compound packet buffer.
		REQUIRE(rtcpCommon::FactoryBuffer[0] == 0xAA);
		REQUIRE(rtcpCommon::FactoryBuffer[19] == 0xAA);

		// XR packet, length: 8 bytes, which fits in the compound packet buffer but
		// has an Extended Report block being built in place within it (so it needs
		// consolidation).
		std::unique_ptr<RTC::RTCP::XrPacket> packet2{ RTC::RTCP::XrPacket::Factory(
			rtcpCommon::FactoryBuffer + 2000, 1000) };

		auto* const rrtExtendedReport =
		  packet2->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

		REQUIRE(packet2->NeedsConsolidation() == true);
		REQUIRE_THROWS_AS(compoundPacket->AddPacket(packet2.get()), MediaSoupError);
		REQUIRE(compoundPacket->GetLength() == 0);
		REQUIRE(compoundPacket->GetPacketsCount() == 0);

		rrtExtendedReport->Discard();
	}

	SECTION(
	  "CompoundPacket::BuildPacketInPlace() and AddPacket() throw if the CompoundPacket needs consolidation")
	{
		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket{ RTC::RTCP::CompoundPacket::Factory(
			rtcpCommon::FactoryBuffer, 1000) };

		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		const auto* const packet1 = compoundPacket->BuildPacketInPlace<RTC::RTCP::ByePacket>();

		REQUIRE(compoundPacket->NeedsConsolidation() == true);

		// We didn't call packet1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(compoundPacket->BuildPacketInPlace<RTC::RTCP::PliPacket>(), MediaSoupError);

		const std::unique_ptr<RTC::RTCP::PliPacket> packet2{ RTC::RTCP::PliPacket::Factory(
			rtcpCommon::FactoryBuffer + 1000, 1000) };

		// We didn't call packet1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(compoundPacket->AddPacket(packet2.get()), MediaSoupError);

		packet1->Consolidate();

		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		// This shouldn't throw now.
		compoundPacket->AddPacket(packet2.get());

		REQUIRE(compoundPacket->NeedsConsolidation() == false);
		REQUIRE(compoundPacket->GetLength() == 16);
		REQUIRE(compoundPacket->GetPacketsCount() == 2);
	}

	SECTION("CompoundPacket::Factory() fails")
	{
		REQUIRE_THROWS_AS(
		  RTC::RTCP::CompoundPacket::Factory(rtcpCommon::FactoryBuffer, 3), MediaSoupTypeError);
	}
}
