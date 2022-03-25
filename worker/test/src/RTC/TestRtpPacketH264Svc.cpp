#include "common.hpp"
#include "helpers.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/Codecs/H264_SVC.hpp"
#include <catch2/catch.hpp>
#include <cstring> // std::memset()
#include <string>
#include <vector>

using namespace RTC;

static uint8_t buffer[65536];
static uint8_t buffer2[65536];

SCENARIO("parse RTP packets with H264 SVC", "[parser][rtp]")
{
	SECTION("parse h264-svc-20.raw")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;
		std::string rid;

		if (!helpers::readBinaryFile("data/H264_SVC/h264-svc-20.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetRidExtensionId(10);
		extenValue = packet->GetExtension(10, extenLen);

		REQUIRE(packet->HasExtension(10) == false);
		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);
		REQUIRE(packet->ReadRid(rid) == false);
		REQUIRE(rid == "");

		auto* payload = packet->GetPayload();

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == true);
		REQUIRE(payloadDescriptor->hasTlIndex == true);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse h264-svc-14.raw")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;
		std::string rid;

		if (!helpers::readBinaryFile("data/H264_SVC/h264-svc-14.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetRidExtensionId(10);
		extenValue = packet->GetExtension(10, extenLen);

		REQUIRE(packet->HasExtension(10) == false);
		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);
		REQUIRE(packet->ReadRid(rid) == false);
		REQUIRE(rid == "");

		auto* payload = packet->GetPayload();

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == true);
		REQUIRE(payloadDescriptor->hasTlIndex == true);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse h264-svc-5.raw")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;
		std::string rid;

		if (!helpers::readBinaryFile("data/H264_SVC/h264-svc-5.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetRidExtensionId(10);
		extenValue = packet->GetExtension(10, extenLen);

		REQUIRE(packet->HasExtension(10) == false);
		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);
		REQUIRE(packet->ReadRid(rid) == false);
		REQUIRE(rid == "");

		auto* payload = packet->GetPayload();

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse h264-svc-1.raw")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;
		std::string rid;

		if (!helpers::readBinaryFile("data/H264_SVC/h264-svc-1.raw", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetRidExtensionId(10);
		extenValue = packet->GetExtension(10, extenLen);

		REQUIRE(packet->HasExtension(10) == false);
		REQUIRE(extenLen == 0);
		REQUIRE(extenValue == nullptr);
		REQUIRE(packet->ReadRid(rid) == false);
		REQUIRE(rid == "");

		auto* payload = packet->GetPayload();

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload));

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I0-7.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I0-7.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xa0);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I0-8.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I0-8.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x00);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 0);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I0-5.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I0-5.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x60);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 0);
		REQUIRE(payloadDescriptor->e == 1);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I1-15.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I1-15.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x80);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I0-14.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I0-14.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xa0);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse I1-20.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/I1-20.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x81);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 1);
		REQUIRE(payloadDescriptor->hasSlIndex);
		REQUIRE(payloadDescriptor->slIndex == 1);
		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == true);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse 2SL-I1.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/2SL-I1.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x31);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 0);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 1);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 1);
		REQUIRE(payloadDescriptor->hasSlIndex);
		REQUIRE(payloadDescriptor->slIndex == 0);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse 2SL-I14.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/2SL-I14.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xa0);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex);
		REQUIRE(payloadDescriptor->slIndex == 0);
		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse 2SL-I20.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/2SL-I20.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0xb0);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 1);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 1);
		REQUIRE(payloadDescriptor->d == 1);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex);
		REQUIRE(payloadDescriptor->slIndex == 1);
		REQUIRE(payloadDescriptor->isKeyFrame == true);
		REQUIRE(payloadDescriptor->hasSlIndex == true);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse 2SL-P20.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/2SL-P20.bin", buffer, &len))
			FAIL("cannot open file");

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 2);
		REQUIRE(extenValue);
		REQUIRE(extenValue[0] == 0x00);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, sizeof(payload), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 0);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex);
		REQUIRE(payloadDescriptor->slIndex == 1);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == true);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}

	SECTION("parse rtp-0.bin")
	{
		size_t len;
		uint8_t extenLen;
		uint8_t* extenValue;

		if (!helpers::readBinaryFile("data/H264_SVC/rtp-0.bin", buffer, &len))
			FAIL("cannot open file");

                int i = 0;
                for(i = 0; i < len; i++)
                {
                        printf(" %" PRIu8 ", ",buffer[i]);
                }

		RtpPacket* packet = RtpPacket::Parse(buffer, len);

		printf("RTP length: %d\n", len);

		if (!packet)
			FAIL("not a RTP packet");

		REQUIRE(packet->HasMarker() == false);
		REQUIRE(packet->HasHeaderExtension() == true);
		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 23617);
		REQUIRE(packet->GetTimestamp() == 1660241882);
		REQUIRE(packet->GetSsrc() == 2674985186);
		REQUIRE(packet->GetHeaderExtensionId() == 0xBEDE);
		REQUIRE(packet->GetHeaderExtensionLength() == 4);
		REQUIRE(packet->HasOneByteExtensions());
		REQUIRE(packet->HasTwoBytesExtensions() == false);

		packet->SetFrameMarkingExtensionId(1);
		extenValue = packet->GetExtension(1, extenLen);

		printf("\nextenValue: [%" PRIu8 "]\n", extenValue[0]);

		REQUIRE(packet->HasExtension(1) == true);
		REQUIRE(extenLen == 1);
		REQUIRE(extenValue);
	//	REQUIRE(extenValue[0] == 0xa0);

		auto* payload = packet->GetPayload();
		RtpPacket::FrameMarking* frameMarking{ nullptr };
		uint8_t frameMarkingLen{ 0 };

		//int i = 0;
		for(i = 0; i < packet->GetPayloadLength(); i++)
		{
			printf(" %" PRIu8 ", ",payload[i]);
		}

		// Read frame-marking.
		packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

		const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, packet->GetPayloadLength(), frameMarking, frameMarkingLen);

		REQUIRE(payloadDescriptor);

		payloadDescriptor->Dump();

		REQUIRE(payloadDescriptor->s == 0);
		REQUIRE(payloadDescriptor->e == 0);
		REQUIRE(payloadDescriptor->i == 0);
		REQUIRE(payloadDescriptor->d == 0);
		REQUIRE(payloadDescriptor->b == 0);
		REQUIRE(payloadDescriptor->hasTlIndex);
		REQUIRE(payloadDescriptor->tlIndex == 0);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->isKeyFrame == false);
		REQUIRE(payloadDescriptor->hasSlIndex == false);
		REQUIRE(payloadDescriptor->hasTlIndex == false);



		delete payloadDescriptor;

		delete packet;
	}
    
    SECTION("create and test RTP files")
	{
		int32_t tmparray[7];

		int32_t pos = 0,i = 0, rows = 0;
		uint8_t type = 0;
		int32_t bytes = 0;
		int32_t sid = -1;
		int32_t tid = -1;
		int32_t isIdr = -1;
		int32_t start = -1;
		int32_t end = -1;
		size_t len;

		std::fstream nf;
		nf.open("test/data/H264_SVC/naluInfo/naluInfo.csv", std::ios::in);
		if (nf.is_open()){
		  std::string line, word;
		  getline(nf, line); // omit the header in the CSV
		  while(getline(nf, line)){
			std::stringstream s(line);
			i = 0;
			while(getline(s, word, ','))
			{
			  tmparray[i++] = std::stoi(word);
			}
			type = tmparray[0];
			bytes = tmparray[1];
			sid = tmparray[2];
			tid = tmparray[3];
			isIdr = tmparray[4];
			start = tmparray[5];
			end = tmparray[6];

			printf("%d,%d,%d,%d,%d,%d,%d\n", type, bytes, sid, tid, isIdr, start, end);

			if(!helpers::readPayloadData("naluInfo.264", pos+4, bytes-4, payload))
			{
				printf("Failed to read payload data!\n");
				break;
			}

			// TODO: One additional byte is written as last value is omitted in the test bench
			std::string strFile1 = "rtp-"+std::to_string(rows)+".bin";
			if(!helpers::writeRtpPacket(strFile1.c_str(), type, bytes-4, sid, tid, isIdr, start, end, payload, buffer2, &len))
			{
				printf("Failed to write RTP packet!\n");
				break;
			}

			RtpPacket* packet = RtpPacket::Parse(buffer2, len);

			if (!packet)
				FAIL("not a RTP packet");

			packet->SetFrameMarkingExtensionId(1);

			auto* payload = packet->GetPayload();
			RtpPacket::FrameMarking* frameMarking{ nullptr };
			uint8_t frameMarkingLen{ 0 };

			// Read frame-marking.
			packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

			const auto* payloadDescriptor = Codecs::H264_SVC::Parse(payload, packet->GetPayloadLength(), frameMarking, frameMarkingLen);

			REQUIRE(payloadDescriptor);

			//payloadDescriptor->Dump();

			pos += bytes;
			rows++;

			delete payloadDescriptor;
			delete packet;

		  }

		  nf.close();
		}
	}
}
