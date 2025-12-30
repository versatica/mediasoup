#include "common.hpp"
#include "testHelpers.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdio>
#include <cstring> // std::memset()
#include <memory>
#include <ostream>

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
		  /*payloadLengh*/ 33,
		  /*hasPadding*/ false,
		  /*paddingLengh*/ 0);
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
		  /*payloadLengh*/ 78,
		  /*hasPadding*/ true,
		  /*paddingLengh*/ 149);
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
		  /*payloadLengh*/ 77,
		  /*hasPadding*/ false,
		  /*paddingLengh*/ 0);
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
		  /*payloadLengh*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLengh*/ 0);
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
			0x10, 0xff, 0x21, 0xff,
			0xff, 0x00, 0x00, 0x33,
			0xff, 0xff, 0xff, 0xff
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
		  /*payloadLengh*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLengh*/ 0);
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
			0x00, 0x00, 0x01, 0x00,
			0x02, 0x01, 0x42, 0x00,
			0x03, 0x02, 0x11, 0x22,
			0x00, 0x00, 0x04, 0x00
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
		  /*payloadLengh*/ 0,
		  /*hasPadding*/ false,
		  /*paddingLengh*/ 0);
	}
}
