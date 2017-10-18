#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTllei.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"

using namespace RTC::RTCP;

SCENARIO("RTCP Feeback RTP parsing", "[parser][rtcp][feedback-rtp]")
{
	SECTION("parse FeedbackRtpNackItem")
	{
		uint8_t buffer[] =
		{
			0x09, 0xc4, 0b10101010, 0b01010101
		};
		uint16_t packetId = 2500;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		FeedbackRtpNackItem* item  = FeedbackRtpNackItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPacketId() == packetId);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);

		delete item;
	}

	SECTION("create FeedbackRtpNackItem")
	{
		uint16_t packetId          = 1;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		// Create local NackItem and check content.
		// FeedbackRtpNackItem();
		FeedbackRtpNackItem item1(packetId, lostPacketBitmask);

		REQUIRE(item1.GetPacketId() == packetId);
		REQUIRE(item1.GetLostPacketBitmask() == lostPacketBitmask);

		// Create local NackItem out of existing one and check content.
		// FeedbackRtpNackItem(FeedbackRtpNackItem*);
		FeedbackRtpNackItem item2(&item1);

		REQUIRE(item2.GetPacketId() == packetId);
		REQUIRE(item2.GetLostPacketBitmask() == lostPacketBitmask);

		// Locally store the content of the packet.
		uint8_t buffer[item2.GetSize()];

		item2.Serialize(buffer);

		// Create local NackItem out of previous packet buffer and check content.
		// FeedbackRtpNackItem(FeedbackRtpNackItem::Header*);
		FeedbackRtpNackItem item3((FeedbackRtpNackItem::Header*)buffer);

		REQUIRE(item3.GetPacketId() == packetId);
		REQUIRE(item3.GetLostPacketBitmask() == lostPacketBitmask);
	}

	SECTION("create FeedbackRtpTmmbrItem")
	{
		uint32_t ssrc     = 1234;
		uint64_t bitrate  = 3000000; // bits per second.
		uint32_t overhead = 1;
		FeedbackRtpTmmbrItem item;

		item.SetSsrc(ssrc);
		item.SetBitrate(bitrate);
		item.SetOverhead(overhead);

		REQUIRE(item.GetSsrc() == ssrc);
		REQUIRE(item.GetBitrate() == bitrate);
		REQUIRE(item.GetOverhead() == overhead);

		uint8_t buffer[8];

		item.Serialize(buffer);

		FeedbackRtpTmmbrItem* item2 = FeedbackRtpTmmbrItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item2);
		REQUIRE(item2->GetSsrc() == ssrc);
		REQUIRE(item2->GetBitrate() == bitrate);
		REQUIRE(item2->GetOverhead() == overhead);

		delete item2;
	}

	SECTION("parse FeedbackRtpTmmbrItem")
	{
		uint8_t buffer[] =
		{
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		uint32_t ssrc     = 3131870413;
		uint64_t bitrate  = 365504;
		uint16_t overhead = 0;
		FeedbackRtpTmmbrItem* item = FeedbackRtpTmmbrItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetBitrate() == bitrate);
		REQUIRE(item->GetOverhead() == overhead);

		delete item;
	}

	SECTION("parse FeedbackRtpTmmbrPacket")
	{

		uint8_t buffer[] =
		{
			0x83, 0xcd, 0x00, 0x04,
			0x6d, 0x6a, 0x8c, 0x9f,
			0x00, 0x00, 0x00, 0x00,
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		uint32_t ssrc     = 3131870413;
		uint64_t bitrate  = 365504;
		uint16_t overhead = 0;
		FeedbackRtpTmmbrPacket* packet = FeedbackRtpTmmbrPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);
		REQUIRE(packet->Begin() != packet->End());

		FeedbackRtpTmmbrItem* item = (*packet->Begin());

		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetBitrate() == bitrate);
		REQUIRE(item->GetOverhead() == overhead);

		delete packet;
	}

	SECTION("parse FeedbackRtpTlleiItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x01, 0b10101010, 0b01010101
		};
		uint16_t packetId = 1;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		FeedbackRtpTlleiItem* item = FeedbackRtpTlleiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPacketId() == packetId);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);

		delete item;
	}

	SECTION("parse FeedbackRtpEcnItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x01, // Extended Highest Sequence Number
			0x00, 0x00, 0x00, 0x01, // ECT (0) Counter
			0x00, 0x00, 0x00, 0x01, // ECT (1) Counter
			0x00, 0x01,             // ECN-CE Counter
			0x00, 0x01,             // not-ECT Counter
			0x00, 0x01,             // Lost Packets Counter
			0x00, 0x01              // Duplication Counter
		};
		uint32_t sequenceNumber = 1;
		uint32_t ect0Counter = 1;
		uint32_t ect1Counter = 1;
		uint16_t ecnCeCounter = 1;
		uint16_t notEctCounter = 1;
		uint16_t lostPackets = 1;
		uint16_t duplicatedPackets = 1;
		FeedbackRtpEcnItem* item = FeedbackRtpEcnItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSequenceNumber() == sequenceNumber);
		REQUIRE(item->GetEct0Counter() == ect0Counter);
		REQUIRE(item->GetEct1Counter() == ect1Counter);
		REQUIRE(item->GetEcnCeCounter() == ecnCeCounter);
		REQUIRE(item->GetNotEctCounter() == notEctCounter);
		REQUIRE(item->GetLostPackets() == lostPackets);
		REQUIRE(item->GetDuplicatedPackets() == duplicatedPackets);

		delete item;
	}
}
