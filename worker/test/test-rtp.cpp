#include "include/catch.hpp"
#include "include/helpers.hpp"
#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <map>

using namespace RTC;

static uint8_t buffer[65536];
static uint8_t buffer2[65536];

SCENARIO("parse RTP packets", "[parser][rtp]")
{
	SECTION("parse packet1.raw")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!Helpers::ReadBinaryFile("data/packet1.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::TO_OFFSET, 1);
		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::RTP_STREAM_ID, 10);

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

		extenValue = packet->GetExtension(RtpHeaderExtensionUri::Type::TO_OFFSET, &extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xff);

		extenValue = packet->GetExtension(RtpHeaderExtensionUri::Type::RTP_STREAM_ID, &extenLen);

		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);

		extenValue = packet->GetExtension(RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION, &extenLen);

		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);

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
		uint8_t extenLen;
		uint8_t* extenValue;
		bool voice;
		uint8_t volume;
		uint32_t absSendTime;

		if (!Helpers::ReadBinaryFile("data/packet3.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, 1);
		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, 3);

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

		extenValue = packet->GetExtension(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, &extenLen);

		REQUIRE(extenLen == 3);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x65);
		REQUIRE(extenValue[1] == 0x34);
		REQUIRE(extenValue[2] == 0x1e);

		extenValue = packet->GetExtension(RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, &extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xd0);

		REQUIRE(packet->ReadAudioLevel(&volume, &voice) == true);
		REQUIRE(volume == 0b1010000);
		REQUIRE(voice == true);
		REQUIRE(packet->ReadAbsSendTime(&absSendTime) == true);
		REQUIRE(absSendTime == 0x65341e);

		std::map<uint8_t, uint8_t> idMapping;

		idMapping[1] = 11;
		idMapping[3] = 13;

		packet->MangleExtensionHeaderIds(idMapping);

		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, 11);
		packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, 13);

		auto clonedPacket = packet->Clone(buffer2);

		delete packet;

		REQUIRE(clonedPacket->HasMarker() == false);
		REQUIRE(clonedPacket->HasExtensionHeader() == true);
		REQUIRE(clonedPacket->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(clonedPacket->GetExtensionHeaderLength() == 8);
		REQUIRE(clonedPacket->GetPayloadType() == 111);
		REQUIRE(clonedPacket->GetSequenceNumber() == 19354);
		REQUIRE(clonedPacket->GetTimestamp() == 863466045);
		REQUIRE(clonedPacket->GetSsrc() == 235797202);
		REQUIRE(clonedPacket->HasOneByteExtensions());
		REQUIRE(!clonedPacket->HasTwoBytesExtensions());

		extenValue = clonedPacket->GetExtension(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, &extenLen);

		REQUIRE(extenLen == 3);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x65);
		REQUIRE(extenValue[1] == 0x34);
		REQUIRE(extenValue[2] == 0x1e);

		extenValue = clonedPacket->GetExtension(RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, &extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xd0);

		REQUIRE(clonedPacket->ReadAudioLevel(&volume, &voice) == true);
		REQUIRE(volume == 0b1010000);
		REQUIRE(voice == true);
		REQUIRE(clonedPacket->ReadAbsSendTime(&absSendTime) == true);
		REQUIRE(absSendTime == 0x65341e);

		delete clonedPacket;
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

	SECTION("rtx encryption-decryption")
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

		uint8_t rtxPayloadType = 102;
		uint32_t rtxSsrc = 6;
		uint16_t rtxSeq = 80;

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
		REQUIRE(packet->GetPayloadLength() == 0);
		REQUIRE(!packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions());

		static uint8_t RtxBuffer[MtuSize];

		auto rtxPacket = packet->Clone(RtxBuffer);

		rtxPacket->RtxEncode(rtxPayloadType, rtxSsrc, rtxSeq);

		REQUIRE(rtxPacket->HasMarker() == false);
		REQUIRE(rtxPacket->HasExtensionHeader() == true);
		REQUIRE(rtxPacket->GetExtensionHeaderLength() == 12);
		REQUIRE(rtxPacket->GetPayloadType() == rtxPayloadType);
		REQUIRE(rtxPacket->GetSequenceNumber() == rtxSeq);
		REQUIRE(rtxPacket->GetTimestamp() == 4);
		REQUIRE(rtxPacket->GetSsrc() == rtxSsrc);
		REQUIRE(rtxPacket->GetPayloadLength() == 2);
		REQUIRE(!rtxPacket->HasOneByteExtensions());
		REQUIRE(rtxPacket->HasTwoBytesExtensions());

		rtxPacket->RtxDecode(1, 5);

		REQUIRE(rtxPacket->HasMarker() == false);
		REQUIRE(rtxPacket->HasExtensionHeader() == true);
		REQUIRE(rtxPacket->GetExtensionHeaderLength() == 12);
		REQUIRE(rtxPacket->GetPayloadType() == 1);
		REQUIRE(rtxPacket->GetSequenceNumber() == 8);
		REQUIRE(rtxPacket->GetTimestamp() == 4);
		REQUIRE(rtxPacket->GetSsrc() == 5);
		REQUIRE(rtxPacket->GetPayloadLength() == 0);
		REQUIRE(!rtxPacket->HasOneByteExtensions());
		REQUIRE(rtxPacket->HasTwoBytesExtensions());

		delete packet;
		delete rtxPacket;
	}
}
