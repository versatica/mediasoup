#include "common.hpp"
#include "Utils.hpp"
#include "testHelpers.hpp" // IWYU pragma: export in worker/test/include/
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <string>

using namespace RTC::RTP;

// NOLINTNEXTLINE (clang-tidy readability-function-size)
SCENARIO("RTP Packet", "[serializable][rtp][packet]")
{
	ResetBuffers();

	SECTION("Packet::Parse() packet1.raw succeeds")
	{
		uint8_t buffer[65536];
		size_t bufferLength;

		if (!helpers::ReadBinaryFile("data/packet1.raw", buffer, std::addressof(bufferLength)))
		{
			FAIL("cannot open file");
		}

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, bufferLength) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ bufferLength,
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 23617,
		  /*timestamp*/ 1660241882,
		  /*ssrc*/ 2674985186,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 4,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 33,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 23617,
		  /*timestamp*/ 1660241882,
		  /*ssrc*/ 2674985186,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 4,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 33,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 23617,
		  /*timestamp*/ 1660241882,
		  /*ssrc*/ 2674985186,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 4,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 33,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 16);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength - 33 + 16,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 23617,
		  /*timestamp*/ 1660241882,
		  /*ssrc*/ 2674985186,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 4,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 16,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 16) ==
		  true);
	}

	SECTION("Packet::Parse() packet2.raw succeeds")
	{
		uint8_t buffer[65536];
		size_t bufferLength;

		if (!helpers::ReadBinaryFile("data/packet2.raw", buffer, std::addressof(bufferLength)))
		{
			FAIL("cannot open file");
		}

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, bufferLength) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ bufferLength,
		  /*length*/ bufferLength,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 28478,
		  /*timestamp*/ 172320136,
		  /*ssrc*/ 3316375386,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 78,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 149);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 28478,
		  /*timestamp*/ 172320136,
		  /*ssrc*/ 3316375386,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 78,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 149);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 28478,
		  /*timestamp*/ 172320136,
		  /*ssrc*/ 3316375386,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 78,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 149);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 16);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength - 78 + 16 - 149,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 28478,
		  /*timestamp*/ 172320136,
		  /*ssrc*/ 3316375386,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 16,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 16) ==
		  true);
	}

	SECTION("Packet::Parse() packet3.raw succeeds")
	{
		uint8_t buffer[65536];
		size_t bufferLength;

		if (!helpers::ReadBinaryFile("data/packet3.raw", buffer, std::addressof(bufferLength)))
		{
			FAIL("cannot open file");
		}

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, bufferLength) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ bufferLength,
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 19354,
		  /*timestamp*/ 863466045,
		  /*ssrc*/ 235797202,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 8,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 77,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 19354,
		  /*timestamp*/ 863466045,
		  /*ssrc*/ 235797202,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 8,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 77,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 19354,
		  /*timestamp*/ 863466045,
		  /*ssrc*/ 235797202,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 8,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 77,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 16);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength - 77 + 16,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 19354,
		  /*timestamp*/ 863466045,
		  /*ssrc*/ 235797202,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 8,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 16,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 16) ==
		  true);
	}

	SECTION("Packet::Parse() without extensions or payload succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x80, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 16);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer) + 16,
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 16,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 16) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);
	}

	SECTION("Packet::Parse() with One-Byte extensions succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x90, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05,
			0xbe, 0xde, 0x00, 0x03, // Header Extension
			0x10, 0xaa, 0x21, 0xbb, // - id: 1, len: 1
			0xff, 0x00, 0x00, 0x33, // - id: 2, len: 2
			0xff, 0xff, 0xff, 0xff, // - id: 3, len: 4
			0x12, 0x23
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 2,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, buffer + 17, 1) == true);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, buffer + 19, 2) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, buffer + 24, 4) == true);

		REQUIRE(packet->HasExtension(4) == false);
		REQUIRE(packet->GetExtensionValue(4, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 2,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, SerializeBuffer + 17, 1) == true);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, SerializeBuffer + 19, 2) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, SerializeBuffer + 24, 4) == true);

		REQUIRE(packet->HasExtension(4) == false);
		REQUIRE(packet->GetExtensionValue(4, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 2,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, CloneBuffer + 17, 1) == true);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, CloneBuffer + 19, 2) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, CloneBuffer + 24, 4) == true);

		REQUIRE(packet->HasExtension(4) == false);
		REQUIRE(packet->GetExtensionValue(4, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 16);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer) - 2 + 16,
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 16,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 16) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);
	}

	SECTION("Packet::Parse() with Two-Bytes extensions succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x90, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05,
			0x10, 0x00, 0x00, 0x04, // Header Extension
			0x00, 0x00, 0x01, 0x00, // - id: 1, len: 0
			0x02, 0x01, 0x42, 0x00, // - id: 2, len: 1
			0x03, 0x02, 0x11, 0x22, // - id: 3, len: 2
			0x00, 0x00, 0x04, 0x00  // - id: 4, len: 0
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 16,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, buffer + 22, 1) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, buffer + 26, 2) == true);

		REQUIRE(packet->HasExtension(4) == true);
		extensionValue = packet->GetExtensionValue(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(5) == false);
		REQUIRE(packet->GetExtensionValue(5, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 16,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, SerializeBuffer + 22, 1) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, SerializeBuffer + 26, 2) == true);

		REQUIRE(packet->HasExtension(4) == true);
		extensionValue = packet->GetExtensionValue(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(5) == false);
		REQUIRE(packet->GetExtensionValue(5, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 16,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, CloneBuffer + 22, 1) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, CloneBuffer + 26, 2) == true);

		REQUIRE(packet->HasExtension(4) == true);
		extensionValue = packet->GetExtensionValue(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(5) == false);
		REQUIRE(packet->GetExtensionValue(5, extensionLen) == nullptr);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 15);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer) + 15,
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 8,
		  /*timestamp*/ 4,
		  /*ssrc*/ 5,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 16,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 15,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 15) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == false);
	}

	SECTION("Packet::Parse() padding-only packet succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0xA0, 0x01, 0x00, 0x09,
			0x00, 0x00, 0x00, 0x05,
			0x00, 0x00, 0x00, 0x06,
			0x00, 0x00, 0x00, 0x00, // Padding (8 bytes)
			0x00, 0x00, 0x00, 0x08
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 9,
		  /*timestamp*/ 5,
		  /*ssrc*/ 6,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 8);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 9,
		  /*timestamp*/ 5,
		  /*ssrc*/ 6,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 8);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 9,
		  /*timestamp*/ 5,
		  /*ssrc*/ 6,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 8);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 1);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ packet->GetLength(),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 9,
		  /*timestamp*/ 5,
		  /*ssrc*/ 6,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		/* Pad to 4 bytes. */

		packet->PadTo4Bytes();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ packet->GetLength(),
		  /*payloadType*/ 1,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 9,
		  /*timestamp*/ 5,
		  /*ssrc*/ 6,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);
	}

	SECTION("Packet::Parse() with wrong arguments fails")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			0x90, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05,
			0xbe, 0xde, 0x00, 0x03, // Header Extension
			0x10, 0xaa, 0x21, 0xbb, // - id: 1, len: 1
			0xff, 0x00, 0x00, 0x33, // - id: 2, len: 2
			0xff, 0xff, 0xff, 0xff, // - id: 3, len: 4
			0x12, 0x23
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ nullptr };

		// bufferLength is lower than packetLen.
		REQUIRE_THROWS_AS(
		  packet.reset(Packet::Parse(buffer, sizeof(buffer), sizeof(buffer) - 1)), MediaSoupTypeError);
		REQUIRE(!packet);
	}

	SECTION("Packet::Factory() succeeds")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 0,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		packet->SetPayloadType(100);
		packet->SetMarker(true);
		packet->SetSequenceNumber(12345);
		packet->SetTimestamp(987654321);
		packet->SetSsrc(1234567890);

		std::vector<Packet::Extension> extensions;

		// Extensions:
		//
		// Using One-Byte Extensions:
		// - Header Extension value length: 1 + 1 + 1 + 2 + 1 + 3 = 9 => 12 (padded)
		// - Header Extension length: 4 + 12 = 16
		//
		// Using Two-Bytes Extensions:
		// - Header Extension value length: 2 + 1 + 2 + 2 + 2 + 3 = 12
		// - Header Extension length: 4 + 12 = 16
		//
		// Extension id 1.
		DataBuffer[0] = 11;
		// Extension id 2.
		DataBuffer[1] = 22;
		DataBuffer[2] = 0xAA;
		// Extension id 14.
		DataBuffer[3] = 14;
		DataBuffer[4] = 0xBB;
		DataBuffer[5] = 0xCC;

		extensions.emplace_back(
		  /*type*/ RTC::RtpHeaderExtensionUri::Type::MID,
		  /*id*/ 1,
		  /*len*/ 1,
		  /*value*/ DataBuffer + 0);

		extensions.emplace_back(
		  /*type*/ RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID,
		  /*id*/ 2,
		  /*len*/ 2,
		  /*value*/ DataBuffer + 1);

		extensions.emplace_back(
		  /*type*/ RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID,
		  /*id*/ 14,
		  /*len*/ 3,
		  /*value*/ DataBuffer + 3);

		// Add One-Byte Extensions.
		packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions);

		packet->SetPayload(DataBuffer, 10);
		packet->PadTo4Bytes(); // payload + padding = 12.

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 + 2,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		const uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 + 2,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Clone it. */

		packet.reset(packet->Clone(CloneBuffer, sizeof(CloneBuffer)));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 + 2,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Set payload. */

		packet->SetPayload(DataBuffer, 1);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 1,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		/* Pad to 4 bytes. */

		packet->PadTo4Bytes();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 1 + 3,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Remove Header Extension. */

		packet->RemoveHeaderExtension();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 1 + 3,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		REQUIRE(packet->HasExtension(1) == false);
		REQUIRE(packet->HasExtension(2) == false);
		REQUIRE(packet->HasExtension(3) == false);
		REQUIRE(packet->HasExtension(14) == false);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		// Add Two-Bytes Extensions.
		packet->SetExtensions(Packet::ExtensionsType::TwoBytes, extensions);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 1 + 3,
		  /*payloadType*/ 100,
		  /*hasMarker*/ true,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 1,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[1]);
		REQUIRE(extensionValue[1] == DataBuffer[2]);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == false);

		REQUIRE(packet->HasExtension(14) == true);
		extensionValue = packet->GetExtensionValue(14, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[3]);
		REQUIRE(extensionValue[1] == DataBuffer[4]);
		REQUIRE(extensionValue[2] == DataBuffer[5]);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 1) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);
	}

	SECTION("Packet::SetExtensions() with ExtensionsType::Auto selects best type")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		std::vector<Packet::Extension> extensions;

		// Can fit into One-Byte type Extensions.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::MID, 1, 1, DataBuffer },
		    { RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID, 14, 16, DataBuffer } });
		packet->SetExtensions(Packet::ExtensionsType::Auto, extensions);
		REQUIRE(packet->HasOneByteExtensions());

		// Requires Two-Bytes type Extensions due to id > 14.
		extensions.assign({ { RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME, 15, 2, DataBuffer } });
		packet->SetExtensions(Packet::ExtensionsType::Auto, extensions);
		REQUIRE(packet->HasTwoBytesExtensions());

		// Requires Two-Bytes type Extensions due to length 0.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID, 1, 0, DataBuffer } });
		packet->SetExtensions(Packet::ExtensionsType::Auto, extensions);
		REQUIRE(packet->HasTwoBytesExtensions());

		// Requires Two-Bytes type Extensions due to length > 16.
		extensions.assign({ { RTC::RtpHeaderExtensionUri::Type::TIME_OFFSET, 1, 17, DataBuffer } });
		packet->SetExtensions(Packet::ExtensionsType::Auto, extensions);
		REQUIRE(packet->HasTwoBytesExtensions());
	}

	SECTION("Packet::SetExtensions() with supported extensions succeeds")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		std::vector<Packet::Extension> extensions;

		std::string mid{ "mid-€1" };
		std::string rid{ "r1-ß" };
		uint32_t absSendtime{ 12345678 };
		uint16_t wideSeqNumber{ 5555 };
		uint8_t absSendtimeValue[100]{};
		uint8_t wideSeqNumberValue[100]{};

		Utils::Byte::Set3Bytes(absSendtimeValue, 0, absSendtime);
		Utils::Byte::Set2Bytes(wideSeqNumberValue, 0, wideSeqNumber);

		// clang-format off
		extensions.assign(
			{
				{
					RTC::RtpHeaderExtensionUri::Type::MID,
					1,
					static_cast<uint8_t>(mid.size()),
					reinterpret_cast<uint8_t*>(mid.data())
				},
				{
					RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID,
					2,
					static_cast<uint8_t>(rid.size()),
					reinterpret_cast<uint8_t*>(rid.data())
				},
				{
					RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME,
					3,
					3,
					absSendtimeValue
				},
				{
					RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01,
					4,
					2,
					wideSeqNumberValue
				}
			}
		);
		// clang-format on

		packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions);

		REQUIRE(packet->HasOneByteExtensions());

		std::string readMid;
		std::string readRid;
		uint32_t readAbsSendtime;
		uint16_t readWideSeqNumber;

		REQUIRE(packet->ReadMid(readMid));
		REQUIRE(readMid == mid);
		REQUIRE(packet->ReadRid(readRid));
		REQUIRE(readRid == rid);
		REQUIRE(packet->ReadAbsSendTime(readAbsSendtime));
		REQUIRE(readAbsSendtime == absSendtime);
		REQUIRE(packet->ReadTransportWideCc01(readWideSeqNumber));
		REQUIRE(readWideSeqNumber == wideSeqNumber);

		std::string newMid{ "mid-®2" };
		uint64_t newAbsSendtimeMs{ 999999 };
		uint16_t newWideSeqNumber{ 5556 };

		REQUIRE(packet->UpdateMid(newMid));
		REQUIRE(packet->UpdateAbsSendTime(newAbsSendtimeMs));
		REQUIRE(packet->UpdateTransportWideCc01(newWideSeqNumber));

		REQUIRE(packet->ReadMid(readMid));
		REQUIRE(readMid == newMid);
		REQUIRE(packet->ReadRid(readRid));
		REQUIRE(readRid == rid);
		REQUIRE(packet->ReadAbsSendTime(readAbsSendtime));
		REQUIRE(readAbsSendtime == Utils::Time::TimeMsToAbsSendTime(newAbsSendtimeMs));
		REQUIRE(packet->ReadTransportWideCc01(readWideSeqNumber));
		REQUIRE(readWideSeqNumber == newWideSeqNumber);

		std::unique_ptr<Packet> packet2{ Packet::Parse(packet->GetBuffer(), packet->GetLength()) };

		REQUIRE(packet2);
		REQUIRE(packet2->Validate(/*storeExtensions*/ false));

		packet2->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		RTC::RtpHeaderExtensionIds headerExtensionIds{};

		headerExtensionIds.mid               = 1;
		headerExtensionIds.rid               = 2;
		headerExtensionIds.absSendTime       = 3;
		headerExtensionIds.transportWideCc01 = 4;

		packet2->AssignExtensionIds(headerExtensionIds);

		REQUIRE(packet2->HasOneByteExtensions());
		REQUIRE(packet2->ReadMid(readMid));
		REQUIRE(readMid == newMid);
		REQUIRE(packet2->ReadRid(readRid));
		REQUIRE(readRid == rid);
		REQUIRE(packet2->ReadAbsSendTime(readAbsSendtime));
		REQUIRE(readAbsSendtime == Utils::Time::TimeMsToAbsSendTime(newAbsSendtimeMs));
		REQUIRE(packet2->ReadTransportWideCc01(readWideSeqNumber));
		REQUIRE(readWideSeqNumber == newWideSeqNumber);
	}

	SECTION("Packet::SetExtensions() fails if wrong extensions are given")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		packet->SetPayload(DataBuffer, 10);
		packet->PadTo4Bytes();

		std::vector<Packet::Extension> extensions;
		auto* d = DataBuffer;

		// Invalid Extension id 0.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::MID, 0, 4, d },
		    { RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID, 1, 1, d } });

		REQUIRE_THROWS_AS(
		  packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions), MediaSoupTypeError);
		REQUIRE_THROWS_AS(
		  packet->SetExtensions(Packet::ExtensionsType::TwoBytes, extensions), MediaSoupTypeError);

		// Invalid Extension id > 14 in One-Byte.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION, 15, 2, d },
		    { RTC::RtpHeaderExtensionUri::Type::MID, 6, 6, d },
		    { RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, 7, 7, d } });

		REQUIRE_THROWS_AS(
		  packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions), MediaSoupTypeError);
		REQUIRE_NOTHROW(packet->SetExtensions(Packet::ExtensionsType::TwoBytes, extensions));

		// Invalid Extension length 0 in One-Byte.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::MID, 3, 0, d },
		    { RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID, 6, 6, d },
		    { RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID, 7, 7, d },
		    { RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL, 8, 8, d } });

		REQUIRE_THROWS_AS(
		  packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions), MediaSoupTypeError);
		REQUIRE_NOTHROW(packet->SetExtensions(Packet::ExtensionsType::TwoBytes, extensions));

		// Invalid Extension length > 16 in One-Byte.
		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::MEDIASOUP_PACKET_ID, 3, 17, d },
		    { RTC::RtpHeaderExtensionUri::Type::MID, 6, 6, d },
		    { RTC::RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION, 7, 7, d },
		    { RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR, 8, 8, d },
		    { RTC::RtpHeaderExtensionUri::Type::PLAYOUT_DELAY, 9, 9, d },
		    { RTC::RtpHeaderExtensionUri::Type::ABS_CAPTURE_TIME, 100, 10, d } });

		REQUIRE_THROWS_AS(
		  packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions), MediaSoupTypeError);
		REQUIRE_NOTHROW(packet->SetExtensions(Packet::ExtensionsType::TwoBytes, extensions));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 4 + 72 + 10 + 2,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 0,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 72,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ true,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		const uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 17);

		REQUIRE(packet->HasExtension(1) == false);

		REQUIRE(packet->HasExtension(6) == true);
		extensionValue = packet->GetExtensionValue(6, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 6);

		REQUIRE(packet->HasExtension(7) == true);
		extensionValue = packet->GetExtensionValue(7, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 7);

		REQUIRE(packet->HasExtension(8) == true);
		extensionValue = packet->GetExtensionValue(8, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 8);

		REQUIRE(packet->HasExtension(9) == true);
		extensionValue = packet->GetExtensionValue(9, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 9);

		REQUIRE(packet->HasExtension(100) == true);
		extensionValue = packet->GetExtensionValue(100, extensionLen);
		REQUIRE(extensionValue[0] == DataBuffer[0]);
		REQUIRE(extensionLen == 10);

		REQUIRE(packet->HasExtension(101) == false);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), DataBuffer, 10) ==
		  true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);
	}

	SECTION("Packet::SetPayload(), SetPayloadLength() and packet::RemovePayload() succeed")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		// clang-format off
		uint8_t payload[] =
		{
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			0x99, 0xAA
		};
		// clang-format on

		/* Set payload. */

		packet->SetPayload(payload, 10);
		packet->PadTo4Bytes();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 10 + 2,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 0,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		/* Set payload length. */

		packet->SetPayloadLength(501);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 501,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 0,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 501,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		/* Remove payload. */

		// This method removes padding.
		packet->RemovePayload();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 0,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		// Invalid arguments.
		REQUIRE_THROWS_AS(packet->SetPayload(nullptr, 2), MediaSoupTypeError);
	}

	SECTION("Packet::ShiftPayload() succeeds")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		packet->SetSsrc(12344321);

		// clang-format off
		uint8_t payload[] =
		{
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			0x99, 0xAA
		};
		// clang-format on

		packet->SetPayload(payload, 10);
		packet->PadTo4Bytes();

		std::vector<Packet::Extension> extensions;

		// One-Byte Extensions:
		// - Header Extension value length: 1 + 1 + 1 + 2 + 1 + 3 = 9 => 12 (padded)
		// - Header Extension length: 4 + 12 = 16
		//
		// clang-format off
		uint8_t extension1[] =
		{
			0x12
		};
		uint8_t extension2[] =
		{
			0x34, 0x56
		};
		uint8_t extension3[] =
		{
			0x78, 0x9A, 0xBC
		};
		// clang-format on

		extensions.assign(
		  { { RTC::RtpHeaderExtensionUri::Type::MID, 1, 1, extension1 },
		    { RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME, 2, 2, extension2 },
		    { RTC::RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01, 3, 3, extension3 } });

		packet->SetExtensions(Packet::ExtensionsType::OneByte, extensions);

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 + 2,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 12344321,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		const uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension1, 1) == true);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension2, 2) == true);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension3, 3) == true);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), payload, 10) == true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* Shift payload. */

		// This method removes padding.
		packet->ShiftPayload(/*payloadOffset*/ 2, /*delta*/ 1);

		// Fill the new byte in the payload with 0XFF.
		packet->GetPayload()[2] = 0xFF;

		// clang-format off
		uint8_t shiftedPayload[] =
		{
			0x11, 0x22, 0xFF, 0x33,
			0x44,	0x55, 0x66, 0x77,
			0x88, 0x99, 0xAA
		};
		// clang-format on

		REQUIRE(packet->Validate(/*storeExtensions*/ false));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 + 1,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 12344321,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 11,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension1, 1) == true);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension2, 2) == true);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension3, 3) == true);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    packet->GetPayload(), packet->GetPayloadLength(), shiftedPayload, 11) == true);
		REQUIRE(packet->IsPaddedTo4Bytes() == false);

		// Reset the payload and padding.
		packet->SetPayload(payload, 10);
		packet->PadTo4Bytes();

		/* Unshift payload. */

		// This method removes padding.
		packet->ShiftPayload(/*payloadOffset*/ 4, /*delta*/ -2);

		// clang-format off
		uint8_t unshiftedPayload[] =
		{
			0x11, 0x22, 0x33, 0x44,
			0x77, 0x88, 0x99, 0xAA
		};
		// clang-format on

		REQUIRE(packet->Validate(/*storeExtensions*/ false));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 16 + 10 - 2,
		  /*payloadType*/ 0,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 0,
		  /*timestamp*/ 0,
		  /*ssrc*/ 12344321,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ true,
		  /*headerExtensionValueLength*/ 12,
		  /*hasOneByteExtensions*/ true,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 8,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtensionValue(1, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension1, 1) == true);
		REQUIRE(extensionLen == 1);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtensionValue(2, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension2, 2) == true);
		REQUIRE(extensionLen == 2);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtensionValue(3, extensionLen);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, extensionLen, extension3, 3) == true);
		REQUIRE(extensionLen == 3);

		REQUIRE(
		  helpers::AreBuffersEqual(
		    packet->GetPayload(), packet->GetPayloadLength(), unshiftedPayload, 8) == true);
		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		// Reset the payload and padding.
		packet->SetPayload(payload, 10);
		packet->PadTo4Bytes();

		/* Shitf and unshift (undo). */

		packet->ShiftPayload(/*payloadOffset*/ 3, /*delta*/ 5);
		packet->ShiftPayload(/*payloadOffset*/ 3, /*delta*/ -5);

		REQUIRE(
		  helpers::AreBuffersEqual(packet->GetPayload(), packet->GetPayloadLength(), payload, 10) == true);
	}

	SECTION("Packet::ShiftPayload() fails if wrong values are given")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		// clang-format off
		uint8_t payload[] =
		{
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			0x99, 0xAA
		};
		// clang-format on

		packet->SetPayload(payload, 10);

		// payloadOffset higger or equal than payload length.
		REQUIRE_THROWS_AS(packet->ShiftPayload(/*payloadOffset*/ 10, /*delta*/ 2), MediaSoupTypeError);

		// delta too big.
		REQUIRE_THROWS_AS(packet->ShiftPayload(/*payloadOffset*/ 2, /*delta*/ -9), MediaSoupTypeError);

		// New computed payload length too big.
		REQUIRE_THROWS_AS(
		  packet->ShiftPayload(/*payloadOffset*/ 2, /*delta*/ packet->GetBufferLength()),
		  MediaSoupTypeError);
	}

	SECTION("Packet::RtxEncode() and packet::RtxDecode() succeed")
	{
		std::unique_ptr<Packet> packet{ Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)) };

		packet->SetPayloadType(100);
		packet->SetSequenceNumber(12345);
		packet->SetTimestamp(987654321);
		packet->SetSsrc(1234567890);

		// clang-format off
		uint8_t payload[] =
		{
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88,
			0x99, 0xAA
		};
		// clang-format on

		packet->SetPayload(payload, 10);
		packet->PadTo4Bytes();

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 10 + 2,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* RTX encode. */

		// This method removes padding.
		packet->RtxEncode(/*payloadType*/ 111, /*ssrc*/ 999999, /*seq*/ 666);

		REQUIRE(packet->Validate(/*storeExtensions*/ false));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 10 + 2,
		  /*payloadType*/ 111,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 666,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 999999,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 12,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == true);

		/* RTX decode. */

		// This method removes padding.
		packet->RtxDecode(/*payloadType*/ 100, /*ssrc*/ 1234567890);

		REQUIRE(packet->Validate(/*storeExtensions*/ false));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ Packet::FixedHeaderMinLength + 10,
		  /*payloadType*/ 100,
		  /*hasMarker*/ false,
		  /*seqNumber*/ 12345,
		  /*timestamp*/ 987654321,
		  /*ssrc*/ 1234567890,
		  /*hasCsrcs*/ false,
		  /*hasHeaderExtension*/ false,
		  /*headerExtensionValueLength*/ 0,
		  /*hasOneByteExtensions*/ false,
		  /*hasTwoBytesExtensions*/ false,
		  /*hasPayload*/ true,
		  /*payloadLength*/ 10,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->IsPaddedTo4Bytes() == false);
	}

	SECTION("Packet::SetBufferReleasedListener() when Packet is destroyed succeeds")
	{
		const size_t bufferLength{ 1200 };
		auto* buffer = new uint8_t[bufferLength];

		std::unique_ptr<Packet> packet{ Packet::Factory(buffer, bufferLength) };

		REQUIRE(packet);

		bool packetBufferReleased{ false };
		bool bufferDeallocated{ false };

		::RTC::Serializable::BufferReleasedListener packetBufferReleasedListener =
		  [&buffer, &packetBufferReleased, &bufferDeallocated](
		    const ::RTC::Serializable* serializable, uint8_t* serializableBuffer)
		{
			packetBufferReleased = true;

			if (serializable->GetBuffer() == buffer)
			{
				delete[] serializableBuffer;
				bufferDeallocated = true;
			}
		};

		packet->SetBufferReleasedListener(std::addressof(packetBufferReleasedListener));

		// If we destroy the Packet it should invoke the listener.
		packet.reset(nullptr);

		REQUIRE(packetBufferReleased == true);
		REQUIRE(bufferDeallocated == true);
	}

	SECTION("Packet::SetBufferReleasedListener() when Packet is serialized into another buffer succeeds")
	{
		const size_t bufferLength{ 1200 };
		auto* buffer = new uint8_t[bufferLength];

		std::unique_ptr<Packet> packet{ Packet::Factory(buffer, bufferLength) };

		REQUIRE(packet);

		bool packetBufferReleased{ false };
		bool bufferDeallocated{ false };

		::RTC::Serializable::BufferReleasedListener packetBufferReleasedListener =
		  [&buffer, &packetBufferReleased, &bufferDeallocated](
		    const ::RTC::Serializable* serializable, uint8_t* serializableBuffer)
		{
			packetBufferReleased = true;

			if (serializable->GetBuffer() == buffer)
			{
				delete[] serializableBuffer;
				bufferDeallocated = true;
			}
		};

		packet->SetBufferReleasedListener(std::addressof(packetBufferReleasedListener));

		// If we serialize the Packet into another buffer it should invoke the
		// listener.
		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		REQUIRE(packetBufferReleased == true);
		REQUIRE(bufferDeallocated == true);

		// NOTE: We need to unset the buffer released listener because once the
		// unique_ptr of the Packet gets out of the scope, the Packet will be
		// deallocated and will invoke the buffer released listener, which at
		// that time is already out of the scope (it's lifetime ended) so it's
		// been destroyed.
		packet->SetBufferReleasedListener(nullptr);
	}
}
