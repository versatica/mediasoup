#include "common.hpp"
#include "catch.hpp"
#include "helpers.hpp"
#include "RTC/RtpPacket.hpp"
#include <cstring> // std::memcmp()
#include <map>
#include <string>

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
		std::string rid;

		if (!helpers::readBinaryFile("data/packet1.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetRidExtensionId(10);
		extenValue = packet->GetExtension(10, extenLen);

		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);
		REQUIRE(packet->ReadRid(rid) == false);
		REQUIRE(rid == "");

		delete packet;
	}

	SECTION("parse packet2.raw")
	{
		size_t len;

		if (!helpers::readBinaryFile("data/packet2.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == false);
		REQUIRE(packet->GetPayloadType() == 100);
		REQUIRE(packet->GetSequenceNumber() == 28478);
		REQUIRE(packet->GetTimestamp() == 172320136);
		REQUIRE(packet->GetSsrc() == 3316375386);
		REQUIRE(packet->GetExtensionHeaderId() == 0);
		REQUIRE(packet->GetExtensionHeaderLength() == 0);
		REQUIRE(packet->HasOneByteExtensions() == false);
		REQUIRE(packet->HasTwoBytesExtensions() == false);

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

		if (!helpers::readBinaryFile("data/packet3.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 19354);
		REQUIRE(packet->GetTimestamp() == 863466045);
		REQUIRE(packet->GetSsrc() == 235797202);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 8);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetAudioLevelExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xd0);
		REQUIRE(packet->ReadAudioLevel(volume, voice) == true);
		REQUIRE(volume == 0b1010000);
		REQUIRE(voice == true);

		packet->SetAbsSendTimeExtensionId(3);
		extenValue = packet->GetExtension(3, extenLen);

		REQUIRE(extenLen == 3);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x65);
		REQUIRE(extenValue[1] == 0x34);
		REQUIRE(extenValue[2] == 0x1e);
		REQUIRE(packet->ReadAbsSendTime(absSendTime) == true);
		REQUIRE(absSendTime == 0x65341e);

		auto clonedPacket = packet->Clone(buffer2);

		delete packet;

		REQUIRE(clonedPacket->HasMarker() == false);
		REQUIRE(clonedPacket->HasExtensionHeader() == true);
		REQUIRE(clonedPacket->GetPayloadType() == 111);
		REQUIRE(clonedPacket->GetSequenceNumber() == 19354);
		REQUIRE(clonedPacket->GetTimestamp() == 863466045);
		REQUIRE(clonedPacket->GetSsrc() == 235797202);
		REQUIRE(clonedPacket->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(clonedPacket->GetExtensionHeaderLength() == 8);
		REQUIRE(clonedPacket->HasOneByteExtensions());
		REQUIRE(clonedPacket->HasTwoBytesExtensions() == false);

		extenValue = clonedPacket->GetExtension(1, extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xd0);
		REQUIRE(clonedPacket->ReadAudioLevel(volume, voice) == true);
		REQUIRE(volume == 0b1010000);
		REQUIRE(voice == true);

		extenValue = clonedPacket->GetExtension(3, extenLen);

		REQUIRE(extenLen == 3);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x65);
		REQUIRE(extenValue[1] == 0x34);
		REQUIRE(extenValue[2] == 0x1e);
		REQUIRE(clonedPacket->ReadAbsSendTime(absSendTime) == true);
		REQUIRE(absSendTime == 0x65341e);

		std::map<uint8_t, uint8_t> idMapping;

		idMapping[1] = 11;
		idMapping[3] = 13;

		clonedPacket->MangleExtensionHeaderIds(idMapping);

		clonedPacket->SetAudioLevelExtensionId(11);
		extenValue = clonedPacket->GetExtension(11, extenLen);

		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xd0);
		REQUIRE(clonedPacket->ReadAudioLevel(volume, voice) == true);
		REQUIRE(volume == 0b1010000);
		REQUIRE(voice == true);

		clonedPacket->SetAbsSendTimeExtensionId(13);
		extenValue = clonedPacket->GetExtension(13, extenLen);

		REQUIRE(extenLen == 3);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x65);
		REQUIRE(extenValue[1] == 0x34);
		REQUIRE(extenValue[2] == 0x1e);
		REQUIRE(clonedPacket->ReadAbsSendTime(absSendTime) == true);
		REQUIRE(absSendTime == 0x65341e);

		REQUIRE(
		  std::memcmp(clonedPacket->GetPayload(), packet->GetPayload(), packet->GetPayloadLength()) == 0);

		delete clonedPacket;
	}

	SECTION("create RtpPacket without extension header")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0b10000000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5
		};
		// clang-format on

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == false);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->HasOneByteExtensions() == false);
		REQUIRE(packet->HasTwoBytesExtensions() == false);
		REQUIRE(packet->GetSsrc() == 5);

		delete packet;
	}

	SECTION("create RtpPacket with One-Byte extension header")
	{
		// clang-format off
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
		// clang-format on

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 12);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		delete packet;
	}

	SECTION("create RtpPacket with Two-Bytes extension header")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0b10010000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5,
			0b00010000, 0, 0, 4, // Extension header
			0, 0, 1, 0,
			2, 1, 0x42, 0,
			3, 2, 0x11, 0x22,
			0, 0, 4, 0
		};
		// clang-format on

		uint8_t extenLen;
		uint8_t* extenValue;

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(packet->GetExtensionHeaderLength() == 16);
		REQUIRE(packet->HasOneByteExtensions() == false);
		REQUIRE(packet->HasTwoBytesExtensions());

		extenValue = packet->GetExtension(1, extenLen);
		REQUIRE(extenValue == nullptr);
		REQUIRE(extenLen == 0);

		extenValue = packet->GetExtension(2, extenLen);
		REQUIRE(extenValue != nullptr);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue[0] == 0x42);

		extenValue = packet->GetExtension(3, extenLen);
		REQUIRE(extenValue != nullptr);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue[0] == 0x11);
		REQUIRE(extenValue[1] == 0x22);

		extenValue = packet->GetExtension(4, extenLen);
		REQUIRE(extenValue == nullptr);
		REQUIRE(extenLen == 0);

		extenValue = packet->GetExtension(5, extenLen);
		REQUIRE(extenValue == nullptr);
		REQUIRE(extenLen == 0);

		delete packet;
	}

	SECTION("rtx encryption-decryption")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0b10010000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5,
			0b00010000, 0, 0, 3, // Extension header
			1, 0, 2, 1,
			0xFF, 0, 3, 4,
			0xFF, 0xFF, 0xFF, 0xFF,
			0x11, 0x11, 0x11, 0x11 // Payload
		};
		// clang-format on

		uint8_t rtxPayloadType{ 102 };
		uint32_t rtxSsrc{ 6 };
		uint16_t rtxSeq{ 80 };

		RtpPacket* packet = RtpPacket::Parse(buffer, sizeof(buffer));

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(packet->GetPayloadLength() == 4);
		REQUIRE(packet->GetExtensionHeaderLength() == 12);
		REQUIRE(packet->HasOneByteExtensions() == false);
		REQUIRE(packet->HasTwoBytesExtensions());

		static uint8_t RtxBuffer[MtuSize];

		auto rtxPacket = packet->Clone(RtxBuffer);

		rtxPacket->RtxEncode(rtxPayloadType, rtxSsrc, rtxSeq);

		REQUIRE(rtxPacket->HasMarker() == false);
		REQUIRE(rtxPacket->HasExtensionHeader() == true);
		REQUIRE(rtxPacket->GetPayloadType() == rtxPayloadType);
		REQUIRE(rtxPacket->GetSequenceNumber() == rtxSeq);
		REQUIRE(rtxPacket->GetTimestamp() == 4);
		REQUIRE(rtxPacket->GetSsrc() == rtxSsrc);
		REQUIRE(rtxPacket->GetPayloadLength() == 6);
		REQUIRE(rtxPacket->GetExtensionHeaderLength() == 12);
		REQUIRE(rtxPacket->HasOneByteExtensions() == false);
		REQUIRE(rtxPacket->HasTwoBytesExtensions());

		rtxPacket->RtxDecode(1, 5);

		REQUIRE(rtxPacket->HasMarker() == false);
		REQUIRE(rtxPacket->HasExtensionHeader() == true);
		REQUIRE(rtxPacket->GetPayloadType() == 1);
		REQUIRE(rtxPacket->GetSequenceNumber() == 8);
		REQUIRE(rtxPacket->GetTimestamp() == 4);
		REQUIRE(rtxPacket->GetSsrc() == 5);
		REQUIRE(rtxPacket->GetPayloadLength() == 4);
		REQUIRE(rtxPacket->GetExtensionHeaderLength() == 12);
		REQUIRE(rtxPacket->HasOneByteExtensions() == false);
		REQUIRE(rtxPacket->HasTwoBytesExtensions());

		delete packet;
		delete rtxPacket;
	}

	SECTION("create RtpPacket and apply payload shift to it")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0b10110000, 0b00000001, 0, 8,
			0, 0, 0, 4,
			0, 0, 0, 5,
			0xBE, 0xDE, 0, 3, // Extension header
			0b00010000, 0xFF, 0b00100001, 0xFF,
			0xFF, 0, 0, 0b00110011,
			0xFF, 0xFF, 0xFF, 0xFF,
			0x00, 0x01, 0x02, 0x03, // Payload
			0x04, 0x05, 0x06, 0x07,
			0x00, 0x00, 0x00, 0x04, // 4 padding bytes
			0x00, 0x00, 0x00, 0x00, // Free buffer
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		size_t len        = 40;
		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->GetPayloadType() == 1);
		REQUIRE(packet->GetSequenceNumber() == 8);
		REQUIRE(packet->GetTimestamp() == 4);
		REQUIRE(packet->GetSsrc() == 5);
		REQUIRE(packet->HasExtensionHeader() == true);
		REQUIRE(packet->GetExtensionHeaderId() == 0xBEDE);
		REQUIRE(packet->GetExtensionHeaderLength() == 12);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);
		REQUIRE(packet->GetPayloadLength() == 8);
		REQUIRE(packet->GetPayloadPadding() == 4);
		REQUIRE(packet->GetSize() == 40);

		auto* payload = packet->GetPayload();

		REQUIRE(payload[0] == 0x00);
		REQUIRE(payload[1] == 0x01);
		REQUIRE(payload[2] == 0x02);
		REQUIRE(payload[3] == 0x03);
		REQUIRE(payload[4] == 0x04);
		REQUIRE(payload[5] == 0x05);
		REQUIRE(payload[6] == 0x06);
		REQUIRE(payload[7] == 0x07);

		packet->ShiftPayload(0, 2, true);

		REQUIRE(packet->GetPayloadLength() == 10);
		REQUIRE(packet->GetPayloadPadding() == 4);
		REQUIRE(packet->GetSize() == 42);
		REQUIRE(payload[2] == 0x00);
		REQUIRE(payload[3] == 0x01);
		REQUIRE(payload[4] == 0x02);
		REQUIRE(payload[5] == 0x03);
		REQUIRE(payload[6] == 0x04);
		REQUIRE(payload[7] == 0x05);
		REQUIRE(payload[8] == 0x06);
		REQUIRE(payload[9] == 0x07);

		packet->ShiftPayload(0, 2, false);

		REQUIRE(packet->GetPayloadLength() == 8);
		REQUIRE(packet->GetPayloadPadding() == 4);
		REQUIRE(packet->GetSize() == 40);
		REQUIRE(payload[0] == 0x00);
		REQUIRE(payload[1] == 0x01);
		REQUIRE(payload[2] == 0x02);
		REQUIRE(payload[3] == 0x03);
		REQUIRE(payload[4] == 0x04);
		REQUIRE(payload[5] == 0x05);
		REQUIRE(payload[6] == 0x06);
		REQUIRE(payload[7] == 0x07);

		packet->ShiftPayload(4, 4, true);

		REQUIRE(packet->GetPayloadLength() == 12);
		REQUIRE(packet->GetPayloadPadding() == 4);
		REQUIRE(packet->GetSize() == 44);
		REQUIRE(payload[0] == 0x00);
		REQUIRE(payload[1] == 0x01);
		REQUIRE(payload[2] == 0x02);
		REQUIRE(payload[3] == 0x03);
		REQUIRE(payload[8] == 0x04);
		REQUIRE(payload[9] == 0x05);
		REQUIRE(payload[10] == 0x06);
		REQUIRE(payload[11] == 0x07);

		delete packet;
	}
}
