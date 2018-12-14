#ifndef MS_UTILS_HPP
#define MS_UTILS_HPP

#include "common.hpp"
#include <string>

namespace Utils
{
	class Byte
	{
	public:
		/**
		 * Getters below get value in Host Byte Order.
		 * Setters below set value in Network Byte Order.
		 */
		static uint8_t Get1Byte(const uint8_t* data, size_t i);
		static uint16_t Get2Bytes(const uint8_t* data, size_t i);
		static uint32_t Get3Bytes(const uint8_t* data, size_t i);
		static uint32_t Get4Bytes(const uint8_t* data, size_t i);
		static uint64_t Get8Bytes(const uint8_t* data, size_t i);
		static void Set1Byte(uint8_t* data, size_t i, uint8_t value);
		static void Set2Bytes(uint8_t* data, size_t i, uint16_t value);
		static void Set3Bytes(uint8_t* data, size_t i, uint32_t value);
		static void Set4Bytes(uint8_t* data, size_t i, uint32_t value);
		static void Set8Bytes(uint8_t* data, size_t i, uint64_t value);
		static uint16_t PadTo4Bytes(uint16_t size);
		static uint32_t PadTo4Bytes(uint32_t size);
	};

	/* Inline static methods. */

	inline uint8_t Byte::Get1Byte(const uint8_t* data, size_t i)
	{
		return data[i];
	}

	inline uint16_t Byte::Get2Bytes(const uint8_t* data, size_t i)
	{
		return uint16_t{ data[i + 1] } | uint16_t{ data[i] } << 8;
	}

	inline uint32_t Byte::Get3Bytes(const uint8_t* data, size_t i)
	{
		return uint32_t{ data[i + 2] } | uint32_t{ data[i + 1] } << 8 | uint32_t{ data[i] } << 16;
	}

	inline uint32_t Byte::Get4Bytes(const uint8_t* data, size_t i)
	{
		return uint32_t{ data[i + 3] } | uint32_t{ data[i + 2] } << 8 | uint32_t{ data[i + 1] } << 16 |
		       uint32_t{ data[i] } << 24;
	}

	inline uint64_t Byte::Get8Bytes(const uint8_t* data, size_t i)
	{
		return uint64_t{ Byte::Get4Bytes(data, i) } << 32 | Byte::Get4Bytes(data, i + 4);
	}

	inline void Byte::Set1Byte(uint8_t* data, size_t i, uint8_t value)
	{
		data[i] = value;
	}

	inline void Byte::Set2Bytes(uint8_t* data, size_t i, uint16_t value)
	{
		data[i + 1] = static_cast<uint8_t>(value);
		data[i]     = static_cast<uint8_t>(value >> 8);
	}

	inline void Byte::Set3Bytes(uint8_t* data, size_t i, uint32_t value)
	{
		data[i + 2] = static_cast<uint8_t>(value);
		data[i + 1] = static_cast<uint8_t>(value >> 8);
		data[i]     = static_cast<uint8_t>(value >> 16);
	}

	inline void Byte::Set4Bytes(uint8_t* data, size_t i, uint32_t value)
	{
		data[i + 3] = static_cast<uint8_t>(value);
		data[i + 2] = static_cast<uint8_t>(value >> 8);
		data[i + 1] = static_cast<uint8_t>(value >> 16);
		data[i]     = static_cast<uint8_t>(value >> 24);
	}

	inline void Byte::Set8Bytes(uint8_t* data, size_t i, uint64_t value)
	{
		data[i + 7] = static_cast<uint8_t>(value);
		data[i + 6] = static_cast<uint8_t>(value >> 8);
		data[i + 5] = static_cast<uint8_t>(value >> 16);
		data[i + 4] = static_cast<uint8_t>(value >> 24);
		data[i + 3] = static_cast<uint8_t>(value >> 32);
		data[i + 2] = static_cast<uint8_t>(value >> 40);
		data[i + 1] = static_cast<uint8_t>(value >> 48);
		data[i]     = static_cast<uint8_t>(value >> 56);
	}

	inline uint16_t Byte::PadTo4Bytes(uint16_t size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFC) + 4;
		else
			return size;
	}

	inline uint32_t Byte::PadTo4Bytes(uint32_t size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFFFFFC) + 4;
		else
			return size;
	}

	class String
	{
	public:
		static void ToLowerCase(std::string& str);
	};

	inline void String::ToLowerCase(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	}

	class Time
	{
		// Seconds from Jan 1, 1900 to Jan 1, 1970.
		static constexpr uint32_t UnixNtpOffset{ 0x83AA7E80 };
		// NTP fractional unit.
		static constexpr uint64_t NtpFractionalUnit{ 1LL << 32 };

	public:
		struct Ntp
		{
			uint32_t seconds;
			uint32_t fractions;
		};

		static Time::Ntp TimeMs2Ntp(uint64_t ms);
		static bool IsNewerTimestamp(uint32_t timestamp, uint32_t prevTimestamp);
		static uint32_t LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2);
	};

	inline Time::Ntp Time::TimeMs2Ntp(uint64_t ms)
	{
		Ntp ntp;

		ntp.seconds = ms / 1000;
		ntp.fractions =
		  static_cast<uint32_t>(static_cast<double>(ms % 1000) / 1000 * Utils::Time::NtpFractionalUnit);

		return ntp;
	}

	inline bool Time::IsNewerTimestamp(uint32_t timestamp, uint32_t prevTimestamp)
	{
		// Distinguish between elements that are exactly 0x80000000 apart.
		// If t1>t2 and |t1-t2| = 0x80000000: IsNewer(t1,t2)=true,
		// IsNewer(t2,t1)=false
		// rather than having IsNewer(t1,t2) = IsNewer(t2,t1) = false.
		if (static_cast<uint32_t>(timestamp - prevTimestamp) == 0x80000000)
			return timestamp > prevTimestamp;

		return timestamp != prevTimestamp &&
		       static_cast<uint32_t>(timestamp - prevTimestamp) < 0x80000000;
	}

	inline uint32_t Time::LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2)
	{
		return IsNewerTimestamp(timestamp1, timestamp2) ? timestamp1 : timestamp2;
	}
} // namespace Utils

#endif
