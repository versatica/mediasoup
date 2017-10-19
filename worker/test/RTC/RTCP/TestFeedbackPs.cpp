#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsLei.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/FeedbackPsRpsi.hpp"
#include "RTC/RTCP/FeedbackPsSli.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"
#include "RTC/RTCP/FeedbackPsVbcm.hpp"
#include <cstring> // std::memcmp()

using namespace RTC::RTCP;

namespace TestFeedbackPsRemb
{
	// RTCP REMB packet.
	uint8_t buffer[] = {
		0x8f, 0xce, 0x00, 0x06, // Type: 206 (Payload Specific), Length: 6
		0xfa, 0x17, 0xfa, 0x17, // Sender SSRC: 0xfa17fa17
		0x00, 0x00, 0x00, 0x00, // Media source SSRC: 0x00000000
		0x52, 0x45, 0x4d, 0x42, // Unique Identifier: REMB
		0x02, 0x01, 0xdf, 0x82, // SSRCs: 2, BR exp: 0, Mantissa: 122754
		0x02, 0xd0, 0x37, 0x02, // SSRC1: 0x02d03702
		0x04, 0xa7, 0x67, 0x47  // SSRC2: 0x04a76747
	};

	// REMB values.
	uint32_t senderSsrc = 0xfa17fa17;
	uint32_t mediaSsrc = 0;
	uint64_t bitrate = 122754;
	std::vector<uint32_t> ssrcs { 0x02d03702, 0x04a76747 };

	void verifyRembPacket(FeedbackPsRembPacket* packet)
	{
		REQUIRE(packet->GetSenderSsrc() == senderSsrc);
		REQUIRE(packet->GetMediaSsrc() == mediaSsrc);
		REQUIRE(packet->GetBitrate() == bitrate);
		REQUIRE(packet->GetSsrcs() == ssrcs);
	}
}

SCENARIO("RTCP Feedback PS parsing", "[parser][rtcp][feedback-ps]")
{
	SECTION("parse FeedbackPsSliItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x08, 0x01, 0x01
		};

		uint16_t first = 1;
		uint16_t number = 4;
		uint8_t pictureId = 1;

		FeedbackPsSliItem* item = FeedbackPsSliItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetFirst() == first);
		REQUIRE(item->GetNumber() == number);
		REQUIRE(item->GetPictureId() == pictureId);

		delete item;
	}

	SECTION("parse FeedbackPsRpsiItem")
	{
		uint8_t buffer[] =
		{
			0x08,                   // Padding Bits
			0x02,                   // Zero | Payload Type
			0x00, 0x00,             // Native RPSI bit string
			0x00, 0x00, 0x01, 0x00
		};

		uint8_t payloadType = 1;
		uint8_t payloadMask = 1;
		size_t length = 5;

		FeedbackPsRpsiItem* item = FeedbackPsRpsiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetBitString()[item->GetLength()-1] & 1) == payloadMask);

		delete item;
	}

	SECTION("parse FeedbackPsFirItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08, 0x00, 0x00, 0x00 // Seq nr.
		};

		uint32_t ssrc = 0;
		uint8_t seq = 8;

		FeedbackPsFirItem* item = FeedbackPsFirItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);

		delete item;
	}

	SECTION("parse FeedbackPsTstnItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08,                   // Seq nr.
			0x00, 0x00, 0x08        // Reserved | Index
		};

		uint32_t ssrc = 0;
		uint8_t seq = 8;
		uint8_t index = 1;

		FeedbackPsTstnItem* item = FeedbackPsTstnItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
		REQUIRE(item->GetIndex() == index);

		delete item;
	}

	SECTION("parse FeedbackPsVbcmItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08,                   // Seq nr.
			0x02,                   // Zero | Payload Vbcm
			0x00, 0x01,             // Length
			0x01,                   // VBCM Octet String
			0x00, 0x00, 0x00        // Padding
		};

		uint32_t ssrc = 0;
		uint8_t seq = 8;
		uint8_t payloadType = 1;
		uint16_t length = 1;
		uint8_t valueMask = 1;

		FeedbackPsVbcmItem* item = FeedbackPsVbcmItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetValue()[item->GetLength() -1] & 1) == valueMask);

		delete item;
	}

	SECTION("parse FeedbackPsLeiItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x01 // SSRC
		};

		uint32_t ssrc = 1;

		FeedbackPsLeiItem* item = FeedbackPsLeiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);

		delete item;
	}

	SECTION("parse FeedbackPsAfbPacket")
	{
		uint8_t buffer[] =
		{
			0x8F, 0xce, 0x00, 0x03, // RTCP common header
			0x00, 0x00, 0x00, 0x00, // Sender SSRC
			0x00, 0x00, 0x00, 0x00, // Media SSRC
			0x00, 0x00, 0x00, 0x01  // Data
		};

		size_t dataSize = 4;
		uint8_t dataBitmask = 1;

		FeedbackPsAfbPacket* packet = FeedbackPsAfbPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);
		REQUIRE(packet->GetApplication() == FeedbackPsAfbPacket::Application::UNKNOWN);

		delete packet;
	}

	SECTION("parse FeedbackPsRembPacket")
	{
		using namespace TestFeedbackPsRemb;

		FeedbackPsRembPacket* packet = FeedbackPsRembPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		verifyRembPacket(packet);

		SECTION("serialize packet instance")
		{
			uint8_t serialized[sizeof(buffer)] = {0};

			packet->Serialize(serialized);

			SECTION("compare serialized packet with original buffer")
			{
				REQUIRE(std::memcmp(buffer, serialized, sizeof(buffer)) == 0);
			}
		}

		delete packet;
	}

	SECTION("create FeedbackPsRembPacket")
	{
		using namespace TestFeedbackPsRemb;

		// Create local report and check content.
		FeedbackPsRembPacket packet(senderSsrc, mediaSsrc);

		packet.SetSsrcs(ssrcs);
		packet.SetBitrate(bitrate);

		verifyRembPacket(&packet);
	}
}
