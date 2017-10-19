#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTllei.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

namespace TestFeedbackRtpNack
{
	// RTCP NACK packet.

	// clang-format off
	uint8_t buffer[] =
	{
		0x81, 0xcd, 0x00, 0x03, // Type: 205 (Generic RTP Feedback), Length: 3
		0x00, 0x00, 0x00, 0x01, // Sender SSRC: 0x00000001
		0x03, 0x30, 0xbd, 0xee, // Media source SSRC: 0x0330bdee
		0x0b, 0x8f, 0x00, 0x03  // NACK PID: 2959, NACK BLP: 0x00000003
	};
	// clang-format on

	// NACK values.
	uint32_t senderSsrc        = 0x00000001;
	uint32_t mediaSsrc         = 0x0330bdee;
	uint16_t pid               = 2959;
	uint16_t lostPacketBitmask = 0x00000003;

	void verifyNackPacket(FeedbackRtpNackPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);

		auto it   = packet->Begin();
		auto item = *it;

		REQUIRE(item->GetPacketId() == pid);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);
	}
}

SCENARIO("RTCP Feeback RTP parsing", "[parser][rtcp][feedback-rtp]")
{
	SECTION("parse FeedbackRtpNackItem")
	{
		using namespace TestFeedbackRtpNack;

		FeedbackRtpNackPacket* packet = FeedbackRtpNackPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		verifyNackPacket(packet);

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = { 0 };

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}

		delete packet;
	}

	SECTION("create FeedbackRtpNackPacket")
	{
		using namespace TestFeedbackRtpNack;

		// Create local packet and check content.
		FeedbackRtpNackPacket packet(senderSsrc, mediaSsrc);
		FeedbackRtpNackItem* item = new FeedbackRtpNackItem(pid, lostPacketBitmask);

		packet.AddItem(item);

		verifyNackPacket(&packet);
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
		// clang-format off
		uint8_t buffer[] =
		{
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		// clang-format on

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
		// clang-format off
		uint8_t buffer[] =
		{
			0x83, 0xcd, 0x00, 0x04,
			0x6d, 0x6a, 0x8c, 0x9f,
			0x00, 0x00, 0x00, 0x00,
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		// clang-format on

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
		// clang-format off
		uint8_t buffer[] =
		{
			0x00, 0x01, 0b10101010, 0b01010101
		};
		// clang-format on

		uint16_t packetId          = 1;
		uint16_t lostPacketBitmask = 0b1010101001010101;

		FeedbackRtpTlleiItem* item = FeedbackRtpTlleiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPacketId() == packetId);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);

		delete item;
	}

	SECTION("parse FeedbackRtpEcnItem")
	{
		// clang-format off
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
		// clang-format on

		uint32_t sequenceNumber    = 1;
		uint32_t ect0Counter       = 1;
		uint32_t ect1Counter       = 1;
		uint16_t ecnCeCounter      = 1;
		uint16_t notEctCounter     = 1;
		uint16_t lostPackets       = 1;
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
