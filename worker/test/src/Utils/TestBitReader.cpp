#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("BitReader", "[utils][bitreader]")
{
	SECTION("it reads from its own copy of the given data")
	{
		uint8_t data[] = { 0xA5, 0x3C };

		Utils::BitReader bitReader(data, sizeof(data));

		// Whatever gave the data is gone.
		std::memset(data, 0, sizeof(data));

		REQUIRE(bitReader.GetData() != data);
		REQUIRE(bitReader.GetLength() == 2);
		REQUIRE(bitReader.GetBits(8) == 0xA5);
		REQUIRE(bitReader.GetBits(8) == 0x3C);
	}

	SECTION("bits are read from the most significant one of the first byte")
	{
		// 1010 0101
		const uint8_t data[] = { 0xA5 };

		Utils::BitReader bitReader(data, sizeof(data));

		REQUIRE(bitReader.GetOffset() == 0);
		REQUIRE(bitReader.GetLeftBits() == 8);

		REQUIRE(bitReader.GetBit() == 1);
		REQUIRE(bitReader.GetBit() == 0);
		REQUIRE(bitReader.GetBit() == 1);
		REQUIRE(bitReader.GetBit() == 0);

		REQUIRE(bitReader.GetOffset() == 4);
		REQUIRE(bitReader.GetLeftBits() == 4);

		REQUIRE(bitReader.GetBit() == 0);
		REQUIRE(bitReader.GetBit() == 1);
		REQUIRE(bitReader.GetBit() == 0);
		REQUIRE(bitReader.GetBit() == 1);

		REQUIRE(bitReader.GetOffset() == 8);
		REQUIRE(bitReader.GetLeftBits() == 0);
	}

	SECTION("bits are read across byte boundaries")
	{
		// 1010 0101 0011 1100
		const uint8_t data[] = { 0xA5, 0x3C };

		Utils::BitReader bitReader(data, sizeof(data));

		REQUIRE(bitReader.GetBits(4) == 0b1010);

		// Bits 4 to 11, which take the end of the first byte and the start of
		// the second one.
		REQUIRE(bitReader.GetBits(8) == 0b01010011);
		REQUIRE(bitReader.GetOffset() == 12);
		REQUIRE(bitReader.GetBits(4) == 0b1100);
		REQUIRE(bitReader.GetLeftBits() == 0);
	}

	SECTION("skipped bits are not read")
	{
		// 1010 0101 0011 1100
		const uint8_t data[] = { 0xA5, 0x3C };

		Utils::BitReader bitReader(data, sizeof(data));

		bitReader.SkipBits(12);

		REQUIRE(bitReader.GetOffset() == 12);
		REQUIRE(bitReader.GetLeftBits() == 4);
		REQUIRE(bitReader.GetBits(4) == 0b1100);
	}

	/**
	 * Callers are expected to check GetLeftBits() themselves, since a zero
	 * cannot be told apart from a real one.
	 */
	SECTION("reading past the data gives zeroes rather than reading past it")
	{
		const uint8_t data[] = { 0xFF };

		Utils::BitReader bitReader(data, sizeof(data));

		REQUIRE(bitReader.GetBits(8) == 0xFF);
		REQUIRE(bitReader.GetLeftBits() == 0);

		REQUIRE(bitReader.GetBit() == 0);
		REQUIRE(bitReader.GetBits(8) == 0);

		// And it stays where the data ended.
		REQUIRE(bitReader.GetOffset() == 8);
		REQUIRE(bitReader.GetLeftBits() == 0);
	}

	/**
	 * Non symmetric unsigned encoded integer, whose values take as many bits as
	 * the number of them requires and one less for the lowest ones.
	 */
	SECTION("ReadNs() reads non symmetric unsigned integers")
	{
		// 0|10|11|000, this is, 0, 1 and 2 encoded with n = 3.
		const uint8_t data[] = { 0b01011000 };

		Utils::BitReader bitReader(data, sizeof(data));

		REQUIRE(bitReader.ReadNs(3) == 0);
		REQUIRE(bitReader.GetOffset() == 1);

		REQUIRE(bitReader.ReadNs(3) == 1);
		REQUIRE(bitReader.GetOffset() == 3);

		REQUIRE(bitReader.ReadNs(3) == 2);
		REQUIRE(bitReader.GetOffset() == 5);
	}

	SECTION("ReadNs() tells when there are not enough bits left")
	{
		const uint8_t data[] = { 0b10000000 };

		Utils::BitReader bitReader(data, sizeof(data));

		bitReader.SkipBits(8);

		REQUIRE(bitReader.GetLeftBits() == 0);
		REQUIRE(bitReader.ReadNs(3) == std::nullopt);
	}
}
