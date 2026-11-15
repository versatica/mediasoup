#include "fuzzer/include/FuzzerUtils.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()
#include <string>

void FuzzerUtils::Fuzz(const uint8_t* data, size_t len)
{
	alignas(4) static thread_local uint8_t dataBuffer[65536];

	// NOTE: The setters below write up to 8 bytes at offset `len`.
	if (len > sizeof(dataBuffer) - 8)
	{
		return;
	}

	// NOTE: We need to copy given data into another buffer because we are gonna
	// write into it.
	std::memcpy(dataBuffer, data, len);

	/* IP class. */

	Utils::IP::GetFamily(std::string(reinterpret_cast<const char*>(dataBuffer), INET6_ADDRSTRLEN / 2));
	Utils::IP::GetFamily(std::string(reinterpret_cast<const char*>(dataBuffer), INET6_ADDRSTRLEN));
	Utils::IP::GetFamily(std::string(reinterpret_cast<const char*>(dataBuffer), INET6_ADDRSTRLEN * 2));

	// Protect with try/catch since throws are legit.
	try
	{
		std::string ip(reinterpret_cast<const char*>(dataBuffer), len);

		Utils::IP::NormalizeIp(ip);
	}
	catch (const MediaSoupError&) // NOLINT(bugprone-empty-catch)
	{
	}

	/* Byte class. */

	Utils::Byte::Get1Byte(dataBuffer, len);
	Utils::Byte::Get2Bytes(dataBuffer, len);
	Utils::Byte::Get3Bytes(dataBuffer, len);
	Utils::Byte::Get4Bytes(dataBuffer, len);
	Utils::Byte::Get8Bytes(dataBuffer, len);
	Utils::Byte::Set1Byte(dataBuffer, len, uint8_t{ 6 });
	Utils::Byte::Set2Bytes(dataBuffer, len, uint16_t{ 66 });
	Utils::Byte::Set3Bytes(dataBuffer, len, uint32_t{ 666 });
	Utils::Byte::Set4Bytes(dataBuffer, len, uint32_t{ 666 });
	Utils::Byte::Set8Bytes(dataBuffer, len, uint64_t{ 6666 });
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
	  static_cast<uint64_t>(len), static_cast<uint64_t>(len) + 1000000);
	Utils::Crypto::GetRandomUInt<size_t>(len, len + 1000000);
	Utils::Crypto::GetRandomString(len);
	Utils::Crypto::GetCRC32(dataBuffer, len);

	/* String class. */

	// Protect with try/catch since throws are legit.
	try
	{
		size_t outLen;

		Utils::String::Base64Encode(dataBuffer, len);
		Utils::String::Base64Decode(dataBuffer, len, outLen);
	}
	catch (const MediaSoupError&) // NOLINT(bugprone-empty-catch)
	{
	}

	/* Time class. */

	const auto ntp = Utils::Time::TimeUsToNtp(static_cast<int64_t>(len));

	Utils::Time::NtpToTimeUs(ntp);
	Utils::Time::CompactNtpIntervalToTimeUs(static_cast<uint32_t>(len));
	Utils::Time::CompactNtpRttToTimeUs(static_cast<uint32_t>(len));
	Utils::Time::TimeUsToAbsSendTime(static_cast<int64_t>(len));
	Utils::Time::Q32x32ToTimeUs(static_cast<int64_t>(len));
	Utils::Time::TimeUsToQ32x32(static_cast<int64_t>(len));
}
