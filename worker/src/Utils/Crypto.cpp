#define MS_CLASS "Utils::Crypto"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace Utils
{
	/* Static. */

	// Reflected CRC-32 (IEEE 802.3) generator polynomial, i.e. 0x04C11DB7 reflected. Used by the
	// STUN FINGERPRINT attribute.
	static constexpr uint32_t Crc32Polynomial{ 0xEDB88320 };

	// Reflected CRC-32C (Castagnoli) generator polynomial. RFC 9260 Appendix A defines it as
	// CRC32C_POLY 0x1EDC6F41; 0x82F63B78 is that same value reflected, which is what the reflected
	// tables emitted by the RFC's own build_crc_table() are built from.
	static constexpr uint32_t Crc32cPolynomial{ 0x82F63B78 };

	/* Class variables. */

	thread_local std::mt19937_64 Crypto::rng;
	thread_local EVP_MAC* Crypto::mac{ nullptr };
	thread_local EVP_MAC_CTX* Crypto::hmacSha1Ctx{ nullptr };
	thread_local uint8_t Crypto::hmacSha1Buffer[SHA_DIGEST_LENGTH];

	/* Static methods. */

	/**
	 * Builds the slice-by-8 lookup tables from the polynomial at compile time, in the same way as
	 * the table generator program listed in RFC 9260 Appendix A.
	 *
	 * Table 0 is the classic byte at a time table and is identical to the crc_c[256] table listed
	 * in that appendix. Each subsequent table holds the result of advancing the previous one by
	 * one further zero byte, which is what allows 8 input bytes to be folded in per iteration. The
	 * RFC only lists table 0, since its sample code is byte at a time.
	 */
	constexpr Crypto::CrcTables Crypto::generateCrcTables(uint32_t polynomial)
	{
		CrcTables tables{};

		for (uint32_t i{ 0u }; i < 256u; ++i)
		{
			uint32_t crc{ i };

			for (size_t bit{ 0u }; bit < 8u; ++bit)
			{
				crc = (crc >> 1) ^ (polynomial & (0u - (crc & 1u)));
			}

			tables[0][i] = crc;
		}

		for (uint32_t i{ 0u }; i < 256u; ++i)
		{
			uint32_t crc{ tables[0][i] };

			for (size_t slice{ 1u }; slice < CrcSlices; ++slice)
			{
				crc              = tables[0][crc & 0xFF] ^ (crc >> 8);
				tables[slice][i] = crc;
			}
		}

		return tables;
	}

	/**
	 * Reads a 32 bit little endian word. The slice-by-8 formulation consumes input as little
	 * endian words regardless of host endianness. Written byte wise so that it is both endian
	 * agnostic and safe on unaligned input; compilers fold it into a single load.
	 */
	uint32_t Crypto::ReadLe32(const uint8_t* data)
	{
		return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
		       (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
	}

	/**
	 * Runs the slice-by-8 loop, returning the raw remainder. Callers apply the final transform,
	 * which differs between CRC-32 and CRC-32C.
	 */
	uint32_t Crypto::ComputeCrc(const CrcTables& tables, const uint8_t* data, size_t size)
	{
		uint32_t crc{ 0xFFFFFFFF };

		while (size >= CrcSlices)
		{
			crc ^= Crypto::ReadLe32(data);

			const uint32_t next{ Crypto::ReadLe32(data + 4) };

			crc = tables[7][crc & 0xFF] ^ tables[6][(crc >> 8) & 0xFF] ^ tables[5][(crc >> 16) & 0xFF] ^
			      tables[4][(crc >> 24) & 0xFF] ^ tables[3][next & 0xFF] ^ tables[2][(next >> 8) & 0xFF] ^
			      tables[1][(next >> 16) & 0xFF] ^ tables[0][(next >> 24) & 0xFF];

			data += CrcSlices;
			size -= CrcSlices;
		}

		while (size-- > 0)
		{
			crc = (crc >> 8) ^ tables[0][(crc ^ *data++) & 0xFF];
		}

		return crc;
	}

	void Crypto::ClassInit()
	{
		MS_TRACE();

		std::random_device rd;
		const uint64_t seed = (uint64_t(rd()) << 32) | uint64_t(rd());

		Crypto::rng.seed(seed);

		// Create an OpenSSL HMAC_CTX context for HMAC SHA1 calculation.
		Crypto::mac         = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
		Crypto::hmacSha1Ctx = EVP_MAC_CTX_new(mac);
	}

	void Crypto::ClassDestroy()
	{
		MS_TRACE();

		if (Crypto::hmacSha1Ctx != nullptr)
		{
			EVP_MAC_CTX_free(Crypto::hmacSha1Ctx);
		}

		if (Crypto::mac != nullptr)
		{
			EVP_MAC_free(Crypto::mac);
		}
	}

	std::string Crypto::GetRandomString(size_t len)
	{
		MS_TRACE();

		char buffer[64];
		static constexpr char Chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
		                                  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		                                  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

		len = std::min<size_t>(len, 64);

		for (size_t i{ 0 }; i < len; ++i)
		{
			buffer[i] = Chars[GetRandomUInt<size_t>(0, sizeof(Chars) - 1)];
		}

		return { buffer, len };
	}

	/**
	 * CRC-32 (IEEE 802.3), as used by the STUN FINGERPRINT attribute.
	 */
	uint32_t Crypto::GetCRC32(const uint8_t* data, size_t size)
	{
		MS_TRACE();

		static constexpr CrcTables Tables{ Crypto::generateCrcTables(Crc32Polynomial) };

		// Pin table 0 against entries of the CRC-32 table this file used to carry literally.
		static_assert(Tables[0][0] == 0x00000000, "CRC-32 table entry 0");
		static_assert(Tables[0][1] == 0x77073096, "CRC-32 table entry 1");
		static_assert(Tables[0][2] == 0xee0e612c, "CRC-32 table entry 2");
		static_assert(Tables[0][3] == 0x990951ba, "CRC-32 table entry 3");
		static_assert(Tables[0][254] == 0x5a05df1b, "CRC-32 table entry 254");
		static_assert(Tables[0][255] == 0x2d02ef8d, "CRC-32 table entry 255");

		return ~Crypto::ComputeCrc(Tables, data, size);
	}

	/**
	 * CRC-32C (Castagnoli) as specified in
	 * https://datatracker.ietf.org/doc/html/rfc9260#appendix-A.
	 *
	 * Yields the same values as the generate_crc32c() sample code in that appendix, but computes
	 * them with the slice-by-8 algorithm instead of one byte at a time.
	 */
	uint32_t Crypto::GetCRC32c(const uint8_t* data, size_t size)
	{
		MS_TRACE();

		static constexpr CrcTables Tables{ Crypto::generateCrcTables(Crc32cPolynomial) };

		// Pin table 0 against entries of the crc_c[256] table listed in RFC 9260 Appendix A.
		static_assert(Tables[0][0] == 0x00000000, "RFC 9260 Appendix A crc_c[0]");
		static_assert(Tables[0][1] == 0xF26B8303, "RFC 9260 Appendix A crc_c[1]");
		static_assert(Tables[0][2] == 0xE13B70F7, "RFC 9260 Appendix A crc_c[2]");
		static_assert(Tables[0][3] == 0x1350F3F4, "RFC 9260 Appendix A crc_c[3]");
		static_assert(Tables[0][128] == 0x82F63B78, "RFC 9260 Appendix A crc_c[128]");
		static_assert(Tables[0][254] == 0x5F16D052, "RFC 9260 Appendix A crc_c[254]");
		static_assert(Tables[0][255] == 0xAD7D5351, "RFC 9260 Appendix A crc_c[255]");

		// NOTE: As in the RFC sample code, the result is returned byte swapped.
		const uint32_t result{ ~Crypto::ComputeCrc(Tables, data, size) };
		const uint32_t byte0{ result & 0xff };
		const uint32_t byte1{ (result >> 8) & 0xff };
		const uint32_t byte2{ (result >> 16) & 0xff };
		const uint32_t byte3{ (result >> 24) & 0xff };

		return (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
	}

	const uint8_t* Crypto::GetHmacSha1(const char* key, size_t keyLen, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		int ret;

		OSSL_PARAM sha1[] = {
			{ "digest", OSSL_PARAM_UTF8_STRING, (void*)"sha1", 4, 0 },
      OSSL_PARAM_END
		};

		ret =
		  EVP_MAC_init(Crypto::hmacSha1Ctx, reinterpret_cast<const unsigned char*>(key), keyLen, sha1);

		MS_ASSERT(ret == 1, "OpenSSL EVP_MAC_init() failed with key '%s'", key);

		ret = EVP_MAC_update(Crypto::hmacSha1Ctx, data, len);

		MS_ASSERT(
		  ret == 1, "OpenSSL EVP_MAC_update() failed with key '%s' and data length %zu bytes", key, len);

		size_t resultLen;

		ret = EVP_MAC_final(Crypto::hmacSha1Ctx, Crypto::hmacSha1Buffer, &resultLen, SHA_DIGEST_LENGTH);

		MS_ASSERT(
		  ret == 1, "OpenSSL HMAC_Final() failed with key '%s' and data length %zu bytes", key, len);
		MS_ASSERT(
		  resultLen == SHA_DIGEST_LENGTH, "OpenSSL HMAC_Final() resultLen is %zu instead of 20", resultLen);

		return Crypto::hmacSha1Buffer;
	}

	void Crypto::WriteRandomBytes(uint8_t* buffer, size_t len)
	{
		MS_TRACE();

		if (RAND_bytes(buffer, len) != 1)
		{
			MS_ABORT("OpenSSL RAND_bytes() failed");
		}
	}
} // namespace Utils
