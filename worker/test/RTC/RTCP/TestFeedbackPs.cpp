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

using namespace RTC::RTCP;

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

	SECTION("FeedbackPsRembPacket")
	{
		uint32_t sender_ssrc = 0;
		uint32_t media_ssrc = 0;
		uint64_t bitrate = 654321;
		// Precission lost.
		uint64_t bitrateParsed = 654320;
		std::vector<uint32_t> ssrcs { 11111, 22222, 33333, 44444 };
		// Create a packet.
		FeedbackPsRembPacket packet(sender_ssrc, media_ssrc);

		packet.SetBitrate(bitrate);
		packet.SetSsrcs(ssrcs);

		// Serialize.
		uint8_t rtcpBuffer[RTC::RTCP::BufferSize];

		packet.Serialize(rtcpBuffer);

		RTC::RTCP::Packet::CommonHeader* header = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(rtcpBuffer);
		size_t len = (size_t)(ntohs(header->length) + 1) * 4;

		// Recover the packet out of the serialized buffer.
		FeedbackPsRembPacket* parsed = FeedbackPsRembPacket::Parse(rtcpBuffer, len);

		REQUIRE(parsed);
		REQUIRE(parsed->GetMediaSsrc() == media_ssrc);
		REQUIRE(parsed->GetSenderSsrc() == sender_ssrc);
		REQUIRE(parsed->GetBitrate() == bitrateParsed);
		REQUIRE(parsed->GetSsrcs() == ssrcs);
	}
}
