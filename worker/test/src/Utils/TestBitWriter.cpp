#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcmp(), std::memset()

SCENARIO("BitWriter", "[utils][bitwriter]")
{
	/**
	 * NOTE: The writer only sets the bits it is given, never clears them, so
	 * every buffer below is zeroed first as its callers must do.
	 */
	SECTION("bits are appended from the most significant one of the first byte")
	{
		uint8_t buffer[1]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		REQUIRE(bitWriter.GetOffset() == 0);

		// 1010 0101
		bitWriter.PutBit(1);
		bitWriter.PutBit(0);
		bitWriter.PutBit(1);
		bitWriter.PutBit(0);
		bitWriter.PutBit(0);
		bitWriter.PutBit(1);
		bitWriter.PutBit(0);
		bitWriter.PutBit(1);

		REQUIRE(bitWriter.GetOffset() == 8);
		REQUIRE(buffer[0] == 0xA5);
	}

	SECTION("bits are appended across byte boundaries")
	{
		uint8_t buffer[2]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		bitWriter.PutBits(4, 0b1010);
		// Takes the end of the first byte and the start of the second one.
		bitWriter.PutBits(8, 0b01010011);
		bitWriter.PutBits(4, 0b1100);

		REQUIRE(bitWriter.GetOffset() == 16);
		REQUIRE(buffer[0] == 0xA5);
		REQUIRE(buffer[1] == 0x3C);
	}

	SECTION("what is not written is left as the caller zeroed it")
	{
		uint8_t buffer[2]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		bitWriter.PutBits(5, 0b11111);

		REQUIRE(bitWriter.GetOffset() == 5);
		// 1111 1000
		REQUIRE(buffer[0] == 0xF8);
		REQUIRE(buffer[1] == 0x00);
	}

	SECTION("a range of bits is taken from another buffer")
	{
		// 1010 0101 0011 1100
		const uint8_t src[] = { 0xA5, 0x3C };
		uint8_t buffer[2]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		// Bits 4 to 11, which are not byte aligned at either end.
		bitWriter.PutBitRange(src, sizeof(src), 4, 12);

		REQUIRE(bitWriter.GetOffset() == 8);
		REQUIRE(buffer[0] == 0b01010011);

		// And bits 0 to 3 after them.
		bitWriter.PutBitRange(src, sizeof(src), 0, 4);

		REQUIRE(bitWriter.GetOffset() == 12);
		REQUIRE(buffer[1] == 0b10100000);
	}

	SECTION("a range of bits is appended wherever the writer is at")
	{
		// 1010 0101
		const uint8_t src[] = { 0xA5 };
		uint8_t buffer[1]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		// Leave the writer in the middle of a byte.
		bitWriter.PutBit(1);

		bitWriter.PutBitRange(src, sizeof(src), 0, 4);

		REQUIRE(bitWriter.GetOffset() == 5);
		// 1|1010|000
		REQUIRE(buffer[0] == 0b11010000);
	}

	/**
	 * The fields being copied over are optional, and an absent one delimits an
	 * empty range.
	 */
	SECTION("an empty range writes nothing")
	{
		const uint8_t src[] = { 0xA5 };
		uint8_t buffer[1]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		bitWriter.PutBitRange(src, sizeof(src), 4, 4);

		REQUIRE(bitWriter.GetOffset() == 0);
		REQUIRE(buffer[0] == 0x00);
	}

	SECTION("what is written is read back the same")
	{
		const uint8_t src[] = { 0x80, 0x00, 0xE8 };
		uint8_t buffer[3]{};

		Utils::BitWriter bitWriter(buffer, sizeof(buffer));

		bitWriter.PutBitRange(src, sizeof(src), 0, 24);

		REQUIRE(bitWriter.GetOffset() == 24);
		REQUIRE(std::memcmp(buffer, src, sizeof(src)) == 0);

		Utils::BitReader bitReader(buffer, sizeof(buffer));

		REQUIRE(bitReader.GetBit() == 1);
		REQUIRE(bitReader.GetBit() == 0);
		REQUIRE(bitReader.GetBits(6) == 0);
		REQUIRE(bitReader.GetBits(16) == 232);
	}
}
