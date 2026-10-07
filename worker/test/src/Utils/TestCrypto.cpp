#include "common.hpp"
#include "Utils.hpp"
#include <ankerl/unordered_dense.h>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits()
#include <random> // std::mt19937

SCENARIO("Utils::Crypto", "[utils][crypto]")
{
	SECTION("GetCRC32()")
	{
		uint8_t dataEmpty[] = {};
		uint8_t dataZero[]  = { 0 };
		// clang-format off
		uint8_t dataRandom[] =
		{
			0xFF, 0x00, 0xAB, 0xCD, 0x12, 0x39, 0x54, 0xBB, 0xDD,
			0xEE, 0x01, 0x01, 0x01, 0x01, 0x88, 0x88, 0xAA
		};
		// clang-format on

		REQUIRE(Utils::Crypto::GetCRC32(dataEmpty, sizeof(dataEmpty)) == 0U);
		REQUIRE(Utils::Crypto::GetCRC32(dataZero, sizeof(dataZero)) == 0xD202EF8D);
		REQUIRE(Utils::Crypto::GetCRC32(dataRandom, sizeof(dataRandom)) == 0xEEE31378);
	}

	SECTION("GetCRC32c()")
	{
		// Tests copied from dcSCTP code in libwebrtc:
		// https://webrtc.googlesource.com/src//+/refs/heads/main/net/dcsctp/packet/crc32c_test.cc

		uint8_t dataEmpty[]     = {};
		uint8_t dataZero[]      = { 0 };
		uint8_t dataManyZeros[] = { 0, 0, 0, 0 };
		uint8_t dataShort[]     = { 1, 2, 3, 4 };
		uint8_t dataLong[]      = { 1, 2, 3, 4, 5, 6, 7, 8 };
		// clang-format off
		uint8_t data32Zeros[]   =
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};
		uint8_t data32Ones[] =
		{
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		};
		uint8_t data32Incrementing[] =
		{
			0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
		};
		uint8_t data32Decrementing[] =
		{
			31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
			15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0
		};
		uint8_t dataSCSICommandPDU[] = {
			0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
			0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(Utils::Crypto::GetCRC32c(dataEmpty, sizeof(dataEmpty)) == 0);
		REQUIRE(Utils::Crypto::GetCRC32c(dataZero, sizeof(dataZero)) == 0x51537d52);
		REQUIRE(Utils::Crypto::GetCRC32c(dataManyZeros, sizeof(dataManyZeros)) == 0xC74B6748);
		REQUIRE(Utils::Crypto::GetCRC32c(dataShort, sizeof(dataShort)) == 0xF48C3029);
		REQUIRE(Utils::Crypto::GetCRC32c(dataLong, sizeof(dataLong)) == 0x811F8946);
		// https://tools.ietf.org/html/rfc3720#appendix-B.4
		REQUIRE(Utils::Crypto::GetCRC32c(data32Zeros, sizeof(data32Zeros)) == 0xAA36918A);
		REQUIRE(Utils::Crypto::GetCRC32c(data32Ones, sizeof(data32Ones)) == 0x43ABA862);
		REQUIRE(Utils::Crypto::GetCRC32c(data32Incrementing, sizeof(data32Incrementing)) == 0x4E79DD46);
		REQUIRE(Utils::Crypto::GetCRC32c(data32Decrementing, sizeof(data32Decrementing)) == 0x5CDB3F11);
		REQUIRE(Utils::Crypto::GetCRC32c(dataSCSICommandPDU, sizeof(dataSCSICommandPDU)) == 0x563A96D9);
	}

	SECTION("GetCRC32() and GetCRC32c() match a byte at a time reference")
	{
		// Guards the slice-by-8 implementations against regressions at the 8 byte block
		// boundary and on unaligned input. Deliberately table free so that it shares
		// nothing with the implementations under test.
		auto reference = [](uint32_t polynomial, bool byteSwap, const uint8_t* data, size_t size)
		{
			uint32_t crc{ 0xFFFFFFFF };

			for (size_t i{ 0u }; i < size; ++i)
			{
				uint32_t byte{ static_cast<uint32_t>((crc ^ data[i]) & 0xFF) };

				for (size_t bit{ 0u }; bit < 8u; ++bit)
				{
					byte = (byte & 1u) ? (byte >> 1) ^ polynomial : (byte >> 1);
				}

				crc = (crc >> 8) ^ byte;
			}

			const uint32_t result{ ~crc };

			if (!byteSwap)
			{
				return result;
			}

			return ((result & 0xff) << 24) | (((result >> 8) & 0xff) << 16) |
			       (((result >> 16) & 0xff) << 8) | ((result >> 24) & 0xff);
		};

		std::mt19937 rng{ 20240101 };
		std::array<uint8_t, 1024 + 8> buffer{};

		for (auto& byte : buffer)
		{
			byte = static_cast<uint8_t>(rng());
		}

		size_t mismatches{ 0u };

		// Every length up to 1024 bytes at every misalignment within an 8 byte word.
		for (size_t offset{ 0u }; offset < 8u; ++offset)
		{
			for (size_t size{ 0u }; size <= 1024u; ++size)
			{
				const uint8_t* data = buffer.data() + offset;

				// CRC-32 (IEEE 802.3), no byte swap.
				if (Utils::Crypto::GetCRC32(data, size) != reference(0xEDB88320u, false, data, size))
				{
					++mismatches;
				}

				// CRC-32C (Castagnoli), byte swapped.
				if (Utils::Crypto::GetCRC32c(data, size) != reference(0x82F63B78u, true, data, size))
				{
					++mismatches;
				}
			}
		}

		REQUIRE(mismatches == 0u);
	}
}

SCENARIO("Utils::Crypto::GetRandomUInt()", "[utils][crypto]")
{
	ankerl::unordered_dense::set<uint32_t> randomUint32Numbers;
	ankerl::unordered_dense::set<uint32_t> randomUint64Numbers;

	for (size_t i = 0; i < 200; ++i)
	{
		auto randomNumber =
		  Utils::Crypto::GetRandomUInt<uint32_t>(0, std::numeric_limits<uint32_t>::max());

		REQUIRE(randomUint32Numbers.find(randomNumber) == randomUint32Numbers.end());

		randomUint32Numbers.insert(randomNumber);
	}

	for (size_t i = 0; i < 200; ++i)
	{
		auto randomNumber =
		  Utils::Crypto::GetRandomUInt<uint64_t>(0, std::numeric_limits<uint64_t>::max());

		REQUIRE(randomUint64Numbers.find(randomNumber) == randomUint64Numbers.end());

		randomUint64Numbers.insert(randomNumber);
	}
}
