#include "FuzzerUtils.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memory()
#include <string>

void Fuzzer::Utils::Fuzz(const uint8_t* data, size_t len)
{
	// For some fuzzers below.
	std::unique_ptr<uint8_t[]> data2(new uint8_t[len + (INET6_ADDRSTRLEN * 3)]);

	std::memcpy(data2.get(), data, len);

	/* IP class. */

	std::string ip;

	ip = std::string(reinterpret_cast<const char*>(data2.get()), INET6_ADDRSTRLEN / 2);
	::Utils::IP::GetFamily(ip);

	ip = std::string(reinterpret_cast<const char*>(data2.get()), INET6_ADDRSTRLEN);
	::Utils::IP::GetFamily(ip);

	ip = std::string(reinterpret_cast<const char*>(data2.get()), INET6_ADDRSTRLEN * 2);
	::Utils::IP::GetFamily(ip);

	// Protect with try/catch since throws are legit.
	try
	{
		auto ip = std::string(reinterpret_cast<const char*>(data2.get()), len);

		::Utils::IP::NormalizeIp(ip);
	}
	catch (const MediaSoupError& error)
	{
	}

	/* Byte class. */

	::Utils::Byte::Get1Byte(data2.get(), len);
	::Utils::Byte::Get2Bytes(data2.get(), len);
	::Utils::Byte::Get3Bytes(data2.get(), len);
	::Utils::Byte::Get4Bytes(data2.get(), len);
	::Utils::Byte::Get8Bytes(data2.get(), len);
	::Utils::Byte::Set1Byte(data2.get(), len, uint8_t{ 6u });
	::Utils::Byte::Set2Bytes(data2.get(), len, uint16_t{ 66u });
	::Utils::Byte::Set3Bytes(data2.get(), len, uint32_t{ 666u });
	::Utils::Byte::Set4Bytes(data2.get(), len, uint32_t{ 666u });
	::Utils::Byte::Set8Bytes(data2.get(), len, uint64_t{ 6666u });
	::Utils::Byte::PadTo4Bytes(static_cast<uint8_t>(len));
	::Utils::Byte::PadTo4Bytes(static_cast<uint16_t>(len));
	::Utils::Byte::PadTo4Bytes(static_cast<uint32_t>(len));
	::Utils::Byte::PadTo4Bytes(static_cast<uint64_t>(len));
	::Utils::Byte::PadTo4Bytes(len);

	/* Bits class. */

	::Utils::Bits::CountSetBits(static_cast<uint16_t>(len));

	/* Crypto class. */

	::Utils::Crypto::GetRandomUInt(static_cast<uint32_t>(len), static_cast<uint32_t>(len + 1000000));
	::Utils::Crypto::GetRandomString(len);
	::Utils::Crypto::GetCRC32(data2.get(), len);

	/* String class. */

	// Protect with try/catch since throws are legit.
	try
	{
		size_t outLen;

		::Utils::String::Base64Encode(data2.get(), len);
		::Utils::String::Base64Decode(data2.get(), len, outLen);
	}
	catch (const MediaSoupError& error)
	{
	}

	/* Time class. */

	auto ntp = ::Utils::Time::TimeMs2Ntp(static_cast<uint64_t>(len));

	::Utils::Time::Ntp2TimeMs(ntp);
	::Utils::Time::TimeMsToAbsSendTime(static_cast<uint64_t>(len));
}
