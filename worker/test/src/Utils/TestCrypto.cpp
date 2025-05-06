#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Utils::Crypto::GetCRC32()", "[utils][crypto]")
{
	// clang-format off
	uint8_t dataEmpty[]  = {};
	uint8_t dataZero[]   = { 0 };
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

SCENARIO("Utils::Crypto::GetCRC32c()", "[utils][crypto]")
{
	// Tests copied from dcSCTP code in libwebrtc:
	// https://webrtc.googlesource.com/src//+/refs/heads/main/net/dcsctp/packet/crc32c_test.cc

	// clang-format off
	uint8_t dataEmpty[]     = {};
	uint8_t dataZero[]      = { 0 };
	uint8_t dataManyZeros[] = { 0, 0, 0, 0 };
	uint8_t dataShort[]     = { 1, 2, 3, 4 };
	uint8_t dataLong[]      = { 1, 2, 3, 4, 5, 6, 7, 8 };
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
