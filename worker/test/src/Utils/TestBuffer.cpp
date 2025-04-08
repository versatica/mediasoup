#include "common.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Utils;

SCENARIO("Buffer::DoBuffersOverlap()", "[utils][buffer]")
{
	// 8 bytes long buffer.
	uint8_t buffer[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	size_t length    = sizeof(buffer);
	uint8_t* dstBuffer{ nullptr };

	REQUIRE(length == 8);

	dstBuffer = buffer;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == true);

	dstBuffer = buffer - length - 1;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == false);

	dstBuffer = buffer - length;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == false);

	dstBuffer = buffer - length + 1;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == true);

	dstBuffer = buffer + length + 1;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == false);

	dstBuffer = buffer + length;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == false);

	dstBuffer = buffer + length - 1;
	REQUIRE(Buffer::DoBuffersOverlap(dstBuffer, buffer, length) == true);
}

SCENARIO("Buffer::MemcpyOrMemmove()", "[utils][buffer]")
{
	uint8_t origBuffer[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	uint8_t* buffer{ nullptr };
	size_t length{ 0 };
	uint8_t* dstBuffer{ nullptr };

	SECTION("should use std::memcpy()")
	{
		buffer    = origBuffer;
		length    = 2;
		dstBuffer = origBuffer + 2;

		uint8_t expectedBuffer[] = { 0x00, 0x01, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07 };

		REQUIRE(Buffer::MemcpyOrMemmove(dstBuffer, buffer, length) == false);
		REQUIRE(
		  helpers::areBuffersEqual(
		    origBuffer, sizeof(origBuffer), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("should use std::memcpy()")
	{
		buffer    = origBuffer + 4;
		length    = 2;
		dstBuffer = origBuffer + 2;

		uint8_t expectedBuffer[] = { 0x00, 0x01, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07 };

		REQUIRE(Buffer::MemcpyOrMemmove(dstBuffer, buffer, length) == false);
		REQUIRE(
		  helpers::areBuffersEqual(
		    origBuffer, sizeof(origBuffer), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("should use std::memmove()")
	{
		buffer    = origBuffer;
		length    = 2;
		dstBuffer = origBuffer + 1;

		uint8_t expectedBuffer[] = { 0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07 };

		REQUIRE(Buffer::MemcpyOrMemmove(dstBuffer, buffer, length) == true);
		REQUIRE(
		  helpers::areBuffersEqual(
		    origBuffer, sizeof(origBuffer), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("should use std::memmove()")
	{
		buffer    = origBuffer + 4;
		length    = 2;
		dstBuffer = origBuffer + 3;

		uint8_t expectedBuffer[] = { 0x00, 0x01, 0x02, 0x04, 0x05, 0x05, 0x06, 0x07 };

		REQUIRE(Buffer::MemcpyOrMemmove(dstBuffer, buffer, length) == true);
		REQUIRE(
		  helpers::areBuffersEqual(
		    origBuffer, sizeof(origBuffer), expectedBuffer, sizeof(expectedBuffer)) == true);
	}
}
