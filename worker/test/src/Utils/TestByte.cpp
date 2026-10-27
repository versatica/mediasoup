#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Utils::Byte", "[utils][byte]")
{
	// NOTE: The setup and teardown are implicit in how Catch2 works, meaning that
	// this buffer is initialized before each SECTION below.
	// Docs: https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#test-cases-and-sections

	// clang-format off
	uint8_t buffer[] =
	{
		0b00000000, 0b00000001, 0b00000010, 0b00000011,
		0b10000000, 0b01000000, 0b00100000, 0b00010000,
		0b01111111, 0b11111111, 0b11111111, 0b00000000,
		0b11111111, 0b11111111, 0b11111111, 0b00000000,
		0b10000000, 0b00000000, 0b00000000, 0b00000000
	};
	// clang-format on

	SECTION("Get3Bytes()")
	{
		// Bytes 4,5 and 6 in the array are number 8405024.
		REQUIRE(Utils::Byte::Get3Bytes(buffer, 4) == 8405024);
	}

	SECTION("Set3Bytes()")
	{
		Utils::Byte::Set3Bytes(buffer, 4, 5666777);
		REQUIRE(Utils::Byte::Get3Bytes(buffer, 4) == 5666777);
	}

	SECTION("Get3BytesSigned()")
	{
		// Bytes 8, 9 and 10 in the array are number 8388607 since first bit is 0 and
		// all other bits are 1, so it must be maximum positive 24 bits signed integer,
		// which is Math.pow(2, 23) - 1 = 8388607.
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 8) == 8388607);

		// Bytes 12, 13 and 14 in the array are number -1.
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 12) == -1);

		// Bytes 16, 17 and 18 in the array are number -8388608 since first bit is 1
		// and all other bits are 0, so it must be minimum negative 24 bits signed
		// integer, which is -1 * Math.pow(2, 23) = -8388608.
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 16) == -8388608);
	}

	SECTION("Set3BytesSigned()")
	{
		Utils::Byte::Set3BytesSigned(buffer, 0, 8388607);
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 0) == 8388607);

		Utils::Byte::Set3BytesSigned(buffer, 0, -1);
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 0) == -1);

		Utils::Byte::Set3BytesSigned(buffer, 0, -8388608);
		REQUIRE(Utils::Byte::Get3BytesSigned(buffer, 0) == -8388608);
	}

	SECTION("IsPaddedTo4Bytes()")
	{
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 4 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 252 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint8_t{ 255 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 4 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 252 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 65532 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint16_t{ 65535 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 4 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 252 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 65532 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 4294967292 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint32_t{ 4294967295 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 4 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 252 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 65532 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 4294967292 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 4294967295 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 18446744073709551608u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 18446744073709551612u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(uint64_t{ 18446744073709551615u }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 4 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 252 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 65532 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 4294967292 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 4294967295 }) == false);

// Check if size_t in current host is 64 bits. Otherwise the test would fail.
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 18446744073709551608u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 18446744073709551612u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(size_t{ 18446744073709551615u }) == false);
#endif
	}

	SECTION("IsPaddedTo8Bytes()")
	{
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 4 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 252 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint8_t{ 255 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 4 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 252 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 65532 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint16_t{ 65535 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 4 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 252 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 65532 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 4294967292 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint32_t{ 4294967295 }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 4 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 252 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 65532 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 4294967292 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 4294967295 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 18446744073709551608u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 18446744073709551612u }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(uint64_t{ 18446744073709551615u }) == false);

		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 0 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 1 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 2 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 3 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 4 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 5 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 8 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 9 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 252 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 255 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 256 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 65532 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 65535 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 4294967288 }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 4294967292 }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 4294967295 }) == false);

// Check if size_t in current host is 64 bits. Otherwise the test would fail.
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 18446744073709551608u }) == true);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 18446744073709551612u }) == false);
		REQUIRE(Utils::Byte::IsPaddedTo8Bytes(size_t{ 18446744073709551615u }) == false);
#endif
	}

	SECTION("PadTo4Bytes()")
	{
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 1 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 2 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 3 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 9 }) == 12);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 254 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint8_t{ 255 }) == 0);

		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 1 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 2 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 3 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 9 }) == 12);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint16_t{ 65535 }) == 0);

		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 1 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 2 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 3 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 9 }) == 12);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 4294967292 }) == 4294967292);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint32_t{ 4294967295 }) == 0);

		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 1 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 2 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 3 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 9 }) == 12);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 4294967292 }) == 4294967292);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 4294967295 }) == 4294967296);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 18446744073709551612u }) == 18446744073709551612u);
		REQUIRE(Utils::Byte::PadTo4Bytes(uint64_t{ 18446744073709551615u }) == 0);

		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 1 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 2 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 3 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 9 }) == 12);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 4294967292 }) == 4294967292);

// Check if size_t in current host is 64 bits. Otherwise the test would fail.
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 18446744073709551612u }) == 18446744073709551612u);
		REQUIRE(Utils::Byte::PadTo4Bytes(size_t{ 18446744073709551615u }) == 0);
#endif
	}

	SECTION("PadDownTo4Bytes()")
	{
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 1 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 2 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 3 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 5 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 9 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 15 }) == 12);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 254 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint8_t{ 255 }) == 252);

		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 1 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 2 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 3 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 5 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 9 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 15 }) == 12);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 254 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 255 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint16_t{ 65535 }) == 65532);

		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 1 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 2 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 3 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 5 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 9 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 15 }) == 12);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 254 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 255 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 65535 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 4294967292 }) == 4294967292);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint32_t{ 4294967295 }) == 4294967292);

		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 1 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 2 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 3 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 5 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 9 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 15 }) == 12);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 254 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 255 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 65535 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 4294967292 }) == 4294967292);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 4294967295 }) == 4294967292);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 18446744073709551612u }) == 18446744073709551612u);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(uint64_t{ 18446744073709551615u }) == 18446744073709551612u);

		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 1 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 2 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 3 }) == 0);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 4 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 5 }) == 4);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 9 }) == 8);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 15 }) == 12);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 252 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 254 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 255 }) == 252);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 65532 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 65535 }) == 65532);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 4294967292 }) == 4294967292);

// Check if size_t in current host is 64 bits. Otherwise the test would fail.
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 18446744073709551612u }) == 18446744073709551612u);
		REQUIRE(Utils::Byte::PadDownTo4Bytes(size_t{ 18446744073709551615u }) == 18446744073709551612u);
#endif
	}

	SECTION("PadTo8Bytes()")
	{
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 1 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 2 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 3 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 4 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 6 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 7 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 9 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 16 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 17 }) == 24);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 252 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 254 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint8_t{ 255 }) == 0);

		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 1 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 2 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 3 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 4 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 6 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 7 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 9 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 16 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 17 }) == 24);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 252 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 65532 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint16_t{ 65535 }) == 0);

		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 1 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 2 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 3 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 4 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 6 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 7 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 9 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 16 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 252 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 65532 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 4294967292 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint32_t{ 4294967295 }) == 0);

		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 1 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 2 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 3 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 4 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 6 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 7 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 9 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 16 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 252 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 65532 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 4294967288 }) == 4294967288);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 4294967292 }) == 4294967296);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 4294967295 }) == 4294967296);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 18446744073709551612u }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(uint64_t{ 18446744073709551615u }) == 0);

		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 0 }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 1 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 2 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 3 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 4 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 5 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 6 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 7 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 8 }) == 8);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 9 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 15 }) == 16);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 252 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 254 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 255 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 256 }) == 256);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 65532 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 65535 }) == 65536);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 4294967288 }) == 4294967288);

// Check if size_t in current host is 64 bits. Otherwise the test would fail.
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 4294967292 }) == 4294967296);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 18446744073709551608u }) == 18446744073709551608u);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 18446744073709551612u }) == 0);
		REQUIRE(Utils::Byte::PadTo8Bytes(size_t{ 18446744073709551615u }) == 0);
#endif
	}
}
