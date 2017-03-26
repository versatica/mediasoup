#include "include/catch.hpp"
#include "include/helpers.hpp"
#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpDictionaries.hpp"

using namespace RTC;

static uint8_t buffer[65536];
static uint8_t buffer2[65536];

SCENARIO("parse RTP packets", "[parser][rtp]")
{
	SECTION("parse packet1.raw")
	{
		size_t len;
		uint8_t exten_len;
		uint8_t* exten_value;

		if (!Helpers::ReadBinaryFile("data/packet1.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 4);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());

		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::TO_OFFSET, 1);
		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::RTP_STREAM_ID, 10);

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::TO_OFFSET, &exten_len);

		REQUIRE(exten_len == 1);
		REQUIRE(exten_value);
		REQUIRE(exten_value[0] == 0xff);

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::RTP_STREAM_ID, &exten_len);

		REQUIRE(exten_len == 0);
		REQUIRE(exten_value == nullptr);

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION, &exten_len);

		REQUIRE(exten_len == 0);
		REQUIRE(exten_value == nullptr);

		packet->Serialize(buffer2);

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 4);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::TO_OFFSET, &exten_len);

		REQUIRE(exten_len == 1);
		REQUIRE(exten_value);
		REQUIRE(exten_value[0] == 0xff);

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::RTP_STREAM_ID, &exten_len);

		REQUIRE(exten_len == 0);
		REQUIRE(exten_value == nullptr);

		delete packet;
	}

	SECTION("parse packet2.raw")
	{
		size_t len;

		if (!Helpers::ReadBinaryFile("data/packet2.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == false);
		REQUIRE(packet->GetExtensionHeaderId() == 0);
		REQUIRE(packet->GetExtensionHeaderLength() == 0);
		REQUIRE(packet->GetPayloadType() == 100);
		REQUIRE(packet->GetSequenceNumber() == 28478);
		REQUIRE(packet->GetTimestamp() == 172320136);
		REQUIRE(packet->GetSsrc() == 3316375386);
		REQUIRE(!packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());

		delete packet;
	}

	SECTION("parse packet3.raw")
	{
		size_t len;
		uint8_t exten_len;
		uint8_t* exten_value;

		if (!Helpers::ReadBinaryFile("data/packet3.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 8);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 19354);
		REQUIRE(packet->GetTimestamp() == 863466045);
		REQUIRE(packet->GetSsrc() == 235797202);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());

		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, 3);

		exten_value = packet->GetExtension(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, &exten_len);

		REQUIRE(exten_len == 3);
		REQUIRE(exten_value);
		REQUIRE(exten_value[0] == 0x65);
		REQUIRE(exten_value[1] == 0x34);
		REQUIRE(exten_value[2] == 0x1E);
		REQUIRE(packet->GetAbsSendTime() == 0x65341e);

		auto cloned_packet = packet->Clone(buffer2);

		delete packet;

		REQUIRE(cloned_packet->HasMarker() == false);
		REQUIRE(cloned_packet->HasExtensionHeader() == true);
		REQUIRE(cloned_packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(cloned_packet->GetExtensionHeaderLength() == 8);
		REQUIRE(cloned_packet->GetPayloadType() == 111);
		REQUIRE(cloned_packet->GetSequenceNumber() == 19354);
		REQUIRE(cloned_packet->GetTimestamp() == 863466045);
		REQUIRE(cloned_packet->GetSsrc() == 235797202);
		REQUIRE(cloned_packet->HasOneByteExtensions());
		REQUIRE(!cloned_packet->HasTwoBytesExtensions());

		exten_value = cloned_packet->GetExtension(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, &exten_len);

		REQUIRE(exten_len == 3);
		REQUIRE(exten_value);
		REQUIRE(exten_value[0] == 0x65);
		REQUIRE(exten_value[1] == 0x34);
		REQUIRE(exten_value[2] == 0x1E);
		REQUIRE(cloned_packet->GetAbsSendTime() == 0x65341e);

		delete cloned_packet;
	}

	SECTION("create RtpPacket without extension header")
	{
		uint8_t buffer[] =
		{
			0b10000000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == false);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(!packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());
		REQUIRE(packet->GetSsrc() == 5);

		delete packet;
	}

	SECTION("create RtpPacket with One-Byte extension header")
	{
		uint8_t buffer[] =
		{
			0b10010000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5,
			0xBE, 0xDE, 0, 3, // Extension header
			0b00010000, 0xFF, 0b00100001, 0xFF,
			0xFF, 0, 0, 0b00110011,
			0xFF, 0xFF, 0xFF, 0xFF
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 12);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(!packet->HasTwoBytesExtensions());

		delete packet;
	}

	SECTION("create RtpPacket with Two-Bytes extension header")
	{
		uint8_t buffer[] =
		{
			0b10010000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5,
			0b00010000, 0, 0, 3, // Extension header
			1, 0, 2, 1,
			0xFF, 0, 3, 4,
			0xFF, 0xFF, 0xFF, 0xFF
		};

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderLength() == 12);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(!packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions());

		delete packet;
	}
}
