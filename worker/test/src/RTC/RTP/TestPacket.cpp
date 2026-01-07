#include "common.hpp"
#include "testHelpers.hpp" // IWYU pragma: export in worker/test/include/
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::RTP;

// NOLINTNEXTLINE (clang-tidy readability-function-size)
SCENARIO("RTP Packet", "[rtp][serializable]")
{
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
		  /*frozen*/ true,
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
		  /*frozen*/ false,
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

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*frozen*/ false,
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
		  /*frozen*/ true,
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
		  /*frozen*/ false,
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

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*frozen*/ false,
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
		  /*frozen*/ true,
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
		  /*frozen*/ false,
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

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ bufferLength,
		  /*frozen*/ false,
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
		  /*length*/ 12,
		  /*frozen*/ true,
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

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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

		/* Clone it. */

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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
			0xff, 0xff, 0xff, 0xff  // - id: 3, len: 4
		};
		// clang-format on

		std::unique_ptr<Packet> packet{ Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*frozen*/ true,
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
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		uint8_t* extensionValue;
		uint8_t extensionLen;

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, buffer + 17, 1) == true);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, buffer + 19, 2) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, buffer + 24, 4) == true);

		REQUIRE(packet->HasExtension(4) == false);
		REQUIRE(packet->GetExtension(4, extensionLen) == nullptr);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->HasExtension(1) == true);
		extensionValue = packet->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, SerializeBuffer + 17, 1) == true);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, SerializeBuffer + 19, 2) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, SerializeBuffer + 24, 4) == true);

		REQUIRE(packet->HasExtension(4) == false);
		REQUIRE(packet->GetExtension(4, extensionLen) == nullptr);

		/* Clone it. */

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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
		  /*hasPayload*/ false,
		  /*payloadLength*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(clonedPacket->HasExtension(1) == true);
		extensionValue = clonedPacket->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, CloneBuffer + 17, 1) == true);

		REQUIRE(clonedPacket->HasExtension(2) == true);
		extensionValue = clonedPacket->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, CloneBuffer + 19, 2) == true);

		REQUIRE(clonedPacket->HasExtension(3) == true);
		extensionValue = clonedPacket->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 4);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 4, CloneBuffer + 24, 4) == true);

		REQUIRE(clonedPacket->HasExtension(4) == false);
		REQUIRE(clonedPacket->GetExtension(4, extensionLen) == nullptr);
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
		  /*length*/ 32,
		  /*frozen*/ true,
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
		extensionValue = packet->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, buffer + 22, 1) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, buffer + 26, 2) == true);

		REQUIRE(packet->HasExtension(4) == true);
		extensionValue = packet->GetExtension(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(5) == false);
		REQUIRE(packet->GetExtension(5, extensionLen) == nullptr);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_RTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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
		extensionValue = packet->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(2) == true);
		extensionValue = packet->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, SerializeBuffer + 22, 1) == true);

		REQUIRE(packet->HasExtension(3) == true);
		extensionValue = packet->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, SerializeBuffer + 26, 2) == true);

		REQUIRE(packet->HasExtension(4) == true);
		extensionValue = packet->GetExtension(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(packet->HasExtension(5) == false);
		REQUIRE(packet->GetExtension(5, extensionLen) == nullptr);

		/* Clone it. */

		std::unique_ptr<Packet> clonedPacket{ packet->Clone(CloneBuffer, sizeof(CloneBuffer)) };

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		CHECK_RTP_PACKET(
		  /*packet*/ clonedPacket.get(),
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ sizeof(buffer),
		  /*frozen*/ false,
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

		REQUIRE(clonedPacket->HasExtension(1) == true);
		extensionValue = clonedPacket->GetExtension(1, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(clonedPacket->HasExtension(2) == true);
		extensionValue = clonedPacket->GetExtension(2, extensionLen);
		REQUIRE(extensionLen == 1);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 1, CloneBuffer + 22, 1) == true);

		REQUIRE(clonedPacket->HasExtension(3) == true);
		extensionValue = clonedPacket->GetExtension(3, extensionLen);
		REQUIRE(extensionLen == 2);
		REQUIRE(helpers::AreBuffersEqual(extensionValue, 2, CloneBuffer + 26, 2) == true);

		REQUIRE(clonedPacket->HasExtension(4) == true);
		extensionValue = clonedPacket->GetExtension(4, extensionLen);
		REQUIRE(extensionLen == 0);

		REQUIRE(clonedPacket->HasExtension(5) == false);
		REQUIRE(clonedPacket->GetExtension(5, extensionLen) == nullptr);
	}
}
