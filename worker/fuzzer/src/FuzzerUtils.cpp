#include "FuzzerUtils.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()
#include <string>

namespace
{
	alignas(4) thread_local uint8_t DataBuffer[65536];
}

void FuzzerUtils::Fuzz(const uint8_t* data, size_t len)
{
	// NOTE: We need to copy given data into another buffer because we are gonna
	// write into it.
	std::memcpy(DataBuffer, data, len);

	/* IP class. */

	std::string ip;

	ip = std::string(reinterpret_cast<const char*>(DataBuffer), INET6_ADDRSTRLEN / 2);
	Utils::IP::GetFamily(ip);

	ip = std::string(reinterpret_cast<const char*>(DataBuffer), INET6_ADDRSTRLEN);
	Utils::IP::GetFamily(ip);

	ip = std::string(reinterpret_cast<const char*>(DataBuffer), INET6_ADDRSTRLEN * 2);
	Utils::IP::GetFamily(ip);

	// Protect with try/catch since throws are legit.
	try
	{
		auto ip = std::string(reinterpret_cast<const char*>(DataBuffer), len);

		Utils::IP::NormalizeIp(ip);
	}
	catch (const MediaSoupError& error) // NOLINT(bugprone-empty-catch)
	{
	}

	/* Byte class. */

	Utils::Byte::Get1Byte(DataBuffer, len);
	Utils::Byte::Get2Bytes(DataBuffer, len);
	Utils::Byte::Get3Bytes(DataBuffer, len);
	Utils::Byte::Get4Bytes(DataBuffer, len);
	Utils::Byte::Get8Bytes(DataBuffer, len);
	Utils::Byte::Set1Byte(DataBuffer, len, uint8_t{ 6u });
	Utils::Byte::Set2Bytes(DataBuffer, len, uint16_t{ 66u });
	Utils::Byte::Set3Bytes(DataBuffer, len, uint32_t{ 666u });
	Utils::Byte::Set4Bytes(DataBuffer, len, uint32_t{ 666u });
	Utils::Byte::Set8Bytes(DataBuffer, len, uint64_t{ 6666u });
	Utils::Byte::PadTo4Bytes(static_cast<uint8_t>(len));
	Utils::Byte::PadTo4Bytes(static_cast<uint16_t>(len));
	Utils::Byte::PadTo4Bytes(static_cast<uint32_t>(len));
	Utils::Byte::PadTo4Bytes(static_cast<uint64_t>(len));
	Utils::Byte::PadTo4Bytes(len);
	Utils::Byte::PadTo8Bytes(static_cast<uint8_t>(len));
	Utils::Byte::PadTo8Bytes(static_cast<uint16_t>(len));
	Utils::Byte::PadTo8Bytes(static_cast<uint32_t>(len));
	Utils::Byte::PadTo8Bytes(static_cast<uint64_t>(len));
	Utils::Byte::PadTo8Bytes(len);

	/* Bits class. */

	Utils::Bits::CountSetBits(static_cast<uint16_t>(len));

	/* Crypto class. */

	Utils::Crypto::GetRandomUInt<uint32_t>(
	  static_cast<uint32_t>(len), static_cast<uint32_t>(len + 1000000));
	Utils::Crypto::GetRandomUInt<uint64_t>(
	  static_cast<uint64_t>(len), static_cast<uint64_t>(len + 1000000));
	Utils::Crypto::GetRandomUInt<size_t>(len, len + 1000000);
	Utils::Crypto::GetRandomString(len);
	Utils::Crypto::GetCRC32(DataBuffer, len);

	/* String class. */

	// Protect with try/catch since throws are legit.
	try
	{
		size_t outLen;

		Utils::String::Base64Encode(DataBuffer, len);
		Utils::String::Base64Decode(DataBuffer, len, outLen);
	}
	catch (const MediaSoupError& error) // NOLINT(bugprone-empty-catch)
	{
	}

	/* Time class. */

	auto ntp = Utils::Time::TimeMs2Ntp(static_cast<uint64_t>(len));

	Utils::Time::Ntp2TimeMs(ntp);
	Utils::Time::TimeMsToAbsSendTime(static_cast<uint64_t>(len));
}
