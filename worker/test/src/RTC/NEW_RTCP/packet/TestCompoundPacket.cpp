#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ByePacket.hpp"
#include "RTC/NEW_RTCP/packet/CompoundPacket.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "RTC/NEW_RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/SdesPacket.hpp"
#include "RTC/NEW_RTCP/packet/SenderReportPacket.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPacket.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
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
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::CompoundPacket> compoundPacket{
			RTC::NEW_RTCP::CompoundPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*packetsCount*/ 2);

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ByePacket>() != nullptr);
		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPacket>() != nullptr);

		const auto* const packet1 =
		  static_cast<const RTC::NEW_RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::ByePacket>() == packet1);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::NEW_RTCP::ByePacket*>(packet1),
		  /*buffer*/ buffer,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::BYE,
		  /*unknownType*/ false);

		REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111111, 2222222, 12345678 });
		REQUIRE(packet1->HasReason() == true);
		REQUIRE(packet1->GetReason() == "foobar");
		// Check padding bytes.
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 1] == 0);
		REQUIRE(packet1->GetBuffer()[packet1->GetLength() - 2] == 'r');

		const auto* const packet2 =
		  static_cast<const RTC::NEW_RTCP::UnknownPacket*>(compoundPacket->GetPacketAt(1));

		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::NEW_RTCP::UnknownPacket>() == packet2);

		CHECK_RTCP_PACKET(
		  /*packet*/ const_cast<RTC::NEW_RTCP::UnknownPacket*>(packet2),
		  /*buffer*/ buffer + 24,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*packetType*/ static_cast<RTC::NEW_RTCP::Packet::PacketType>(222),
		  /*unknownType*/ true);

		REQUIRE(packet2->HasUnknownValue() == true);
		REQUIRE(packet2->GetUnknownValueLength() == 8);
		REQUIRE(packet2->GetUnknownValue()[0] == 0x11);
		REQUIRE(packet2->GetUnknownValue()[1] == 0x22);
		REQUIRE(packet2->GetUnknownValue()[7] == 0x88);
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

		REQUIRE(!RTC::NEW_RTCP::CompoundPacket::Parse(buffer1, sizeof(buffer1)));

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

		REQUIRE(!RTC::NEW_RTCP::CompoundPacket::Parse(buffer2, sizeof(buffer2)));

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

		REQUIRE(!RTC::NEW_RTCP::CompoundPacket::Parse(buffer3, sizeof(buffer3)));

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

		REQUIRE(!RTC::NEW_RTCP::CompoundPacket::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("CompoundPacket::Factory() succeeds")
	{
		const auto checkPackets =
		  [](const RTC::NEW_RTCP::CompoundPacket* compoundPacket, const uint8_t* buffer) -> void
		{
			const auto* const packet1 =
			  static_cast<const RTC::NEW_RTCP::ByePacket*>(compoundPacket->GetPacketAt(0));
			const auto* const packet2 =
			  static_cast<const RTC::NEW_RTCP::SdesPacket*>(compoundPacket->GetPacketAt(1));
			const auto sdesItems = packet2->GetItems();
			const auto* const packet3 =
			  static_cast<const RTC::NEW_RTCP::ReceiverReportPacket*>(compoundPacket->GetPacketAt(2));
			const auto reportBlocks = packet3->GetReportBlocks();
			const auto* const packet4 =
			  static_cast<const RTC::NEW_RTCP::SenderReportPacket*>(compoundPacket->GetPacketAt(3));

			REQUIRE(packet1->GetBuffer() == buffer);
			REQUIRE(packet1->GetLength() == 12);
			REQUIRE(packet1->GetSsrcs() == std::vector<uint32_t>{ 1111 });
			REQUIRE(packet1->GetReason() == "foo");

			REQUIRE(packet2->GetBuffer() == buffer + 12);
			REQUIRE(packet2->GetLength() == 16);
			REQUIRE(packet2->GetNumberOfChunks() == 1);
			REQUIRE(sdesItems.size() == 1);
			REQUIRE(sdesItems[0].ssrc == 3333);
			REQUIRE(sdesItems[0].type == RTC::NEW_RTCP::SdesPacket::ItemType::CNAME);
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
			REQUIRE(packet4->GetNtpSec() == 1);
			REQUIRE(packet4->GetNtpFrac() == 2);
			REQUIRE(packet4->GetRtpTimestamp() == 3);
			REQUIRE(packet4->GetPacketCount() == 4);
			REQUIRE(packet4->GetOctetCount() == 5);
			REQUIRE(packet4->GetNumberOfReportBlocks() == 0);
			REQUIRE(packet4->GetReportBlocks().empty());
		};

		std::unique_ptr<RTC::NEW_RTCP::CompoundPacket> compoundPacket{
			RTC::NEW_RTCP::CompoundPacket::Factory(rtcpCommon::FactoryBuffer, 1000)
		};

		REQUIRE(compoundPacket->GetLength() == 0);
		REQUIRE(compoundPacket->GetPacketsCount() == 0);
		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		/* Clone it while empty. */

		const std::unique_ptr<RTC::NEW_RTCP::CompoundPacket> emptyClonedCompoundPacket{
			compoundPacket->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer))
		};

		REQUIRE(emptyClonedCompoundPacket->GetBuffer() == rtcpCommon::CloneBuffer);
		REQUIRE(emptyClonedCompoundPacket->GetLength() == 0);
		REQUIRE(emptyClonedCompoundPacket->GetPacketsCount() == 0);

		/* Build a packet and consolidate it. */

		auto* const packet1 = compoundPacket->BuildPacketInPlace<RTC::NEW_RTCP::ByePacket>();

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

		auto* const packet2 = compoundPacket->BuildPacketInPlace<RTC::NEW_RTCP::ByePacket>();

		packet2->AddSsrc(2222);
		packet2->AddSsrc(2223);

		REQUIRE(compoundPacket->GetLength() == 12);
		REQUIRE(packet2->GetLength() == 12);

		packet2->Discard();

		REQUIRE(compoundPacket->GetLength() == 12);
		REQUIRE(compoundPacket->GetPacketsCount() == 1);
		REQUIRE(compoundPacket->NeedsConsolidation() == false);

		/* Build another packet in the same place and consolidate it. */

		auto* const packet3 = compoundPacket->BuildPacketInPlace<RTC::NEW_RTCP::SdesPacket>();

		REQUIRE(packet3->GetBuffer() == compoundPacket->GetBuffer() + 12);

		packet3->AddChunk(3333);
		packet3->AddItem(RTC::NEW_RTCP::SdesPacket::ItemType::CNAME, "abc");
		packet3->Consolidate();

		// Consolidate() or Discard() can only be called once.
		REQUIRE_THROWS_AS(packet3->Consolidate(), MediaSoupError);
		REQUIRE_THROWS_AS(packet3->Discard(), MediaSoupError);

		/* Build another packet and consolidate it. */

		auto* const packet4 = compoundPacket->BuildPacketInPlace<RTC::NEW_RTCP::ReceiverReportPacket>();

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

		auto* const packet5 = compoundPacket->BuildPacketInPlace<RTC::NEW_RTCP::SenderReportPacket>();

		REQUIRE(packet5->GetBuffer() == compoundPacket->GetBuffer() + 60);

		packet5->SetSsrc(6666);
		packet5->SetNtpSec(1);
		packet5->SetNtpFrac(2);
		packet5->SetRtpTimestamp(3);
		packet5->SetPacketCount(4);
		packet5->SetOctetCount(5);
		packet5->Consolidate();

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ 1000,
		  /*length*/ 88,
		  /*packetsCount*/ 4);

		checkPackets(compoundPacket.get(), rtcpCommon::FactoryBuffer);

		/* Serialize it. */

		compoundPacket->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 88,
		  /*packetsCount*/ 4);

		checkPackets(compoundPacket.get(), rtcpCommon::SerializeBuffer);

		/* Clone it. */

		compoundPacket.reset(
		  compoundPacket->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_COMPOUND_PACKET(
		  /*compoundPacket*/ compoundPacket.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 88,
		  /*packetsCount*/ 4);

		checkPackets(compoundPacket.get(), rtcpCommon::CloneBuffer);
	}

	SECTION("CompoundPacket::Factory() fails")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::CompoundPacket::Factory(rtcpCommon::FactoryBuffer, 3), MediaSoupTypeError);
	}
}
