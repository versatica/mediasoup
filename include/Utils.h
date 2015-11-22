#ifndef MS_UTILS_H
#define	MS_UTILS_H

#include "common.h"
#include <string>
#include <cstring>  // std::memcmp(), std::memcpy()
#include <openssl/hmac.h>

namespace Utils
{
	class IP
	{
	public:
		static int GetFamily(const char *ip, size_t ip_len);
		static int GetFamily(const std::string &ip);
		static void GetAddressInfo(const struct sockaddr* addr, int* family, std::string &ip, MS_PORT* port);
		static bool CompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2);
		static struct sockaddr_storage CopyAddress(const struct sockaddr* addr);
	};

	/* Inline static methods. */

	inline
	int IP::GetFamily(const std::string &ip)
	{
		return GetFamily(ip.c_str(), ip.size());
	}

	inline
	bool IP::CompareAddresses(const struct sockaddr* addr1, const struct sockaddr* addr2)
	{
		// Compare family.
		if (addr1->sa_family != addr2->sa_family || (addr1->sa_family != AF_INET && addr1->sa_family != AF_INET6))
			return false;

		// Compare port.
		if (((struct sockaddr_in*)addr1)->sin_port != ((struct sockaddr_in*)addr2)->sin_port)
			return false;

		// Compare IP.
		switch (addr1->sa_family)
		{
			case AF_INET:
				if (std::memcmp(&((struct sockaddr_in*)addr1)->sin_addr, &((struct sockaddr_in*)addr2)->sin_addr, 4) == 0)
					return true;
				break;
			case AF_INET6:
				if (std::memcmp(&((struct sockaddr_in6*)addr1)->sin6_addr, &((struct sockaddr_in6*)addr2)->sin6_addr, 16) == 0)
					return true;
				break;
			default:
				return false;
		}

		return false;
	}

	inline
	struct sockaddr_storage IP::CopyAddress(const struct sockaddr* addr)
	{
		struct sockaddr_storage copied_addr;

		switch (addr->sa_family)
		{
			case AF_INET:
				std::memcpy(&copied_addr, addr, sizeof(struct sockaddr_in));
				break;
			case AF_INET6:
				std::memcpy(&copied_addr, addr, sizeof(struct sockaddr_in6));
				break;
		}

		return copied_addr;
	}

	class Socket
	{
	public:
		/**
		 * Builds a pair of connected sockets. The sockets are created in non-blocking
		 * mode and with close-exec (so they are closed after fork()).
		 *
		 * @param  family  Socket family (AF_UNIX...).
		 * @param  type    Socket type (SOCK_DGRAM...) + flags.
		 * @param  fds     Array with two integers.
		 *
		 * In case of error throws a MediaSoupError exception.
		 */
		static void BuildSocketPair(int family, int type, int* fds);

	private:
		static bool SetNonBlock(int fd);
		static bool SetCloExec(int fd);
	};

	class File
	{
	public:
		static void CheckFile(const char* file);
	};

	class Byte
	{
	public:
		/**
		 * All these functions get and set bytes in Network Byte Order (Big Endian).
		 */
		static MS_BYTE Get1Byte(const MS_BYTE* data, size_t i);
		static MS_2BYTES Get2Bytes(const MS_BYTE* data, size_t i);
		static MS_4BYTES Get3Bytes(const MS_BYTE* data, size_t i);
		static MS_4BYTES Get4Bytes(const MS_BYTE* data, size_t i);
		static MS_8BYTES Get8Bytes(const MS_BYTE* data, size_t i);
		static void Set1Byte(MS_BYTE* data, size_t i, MS_BYTE value);
		static void Set2Bytes(MS_BYTE* data, size_t i, MS_2BYTES value);
		static void Set3Bytes(MS_BYTE* data, size_t i, MS_4BYTES value);
		static void Set4Bytes(MS_BYTE* data, size_t i, MS_4BYTES value);
		static void Set8Bytes(MS_BYTE* data, size_t i, MS_8BYTES value);
		static MS_2BYTES PadTo4Bytes(MS_2BYTES size);
		static MS_4BYTES PadTo4Bytes(MS_4BYTES size);
	};

	/* Inline static methods. */

	inline
	MS_BYTE Byte::Get1Byte(const MS_BYTE* data, size_t i)
	{
		return data[i];
	}

	inline
	MS_2BYTES Byte::Get2Bytes(const MS_BYTE* data, size_t i)
	{
		return (MS_2BYTES)(data[i+1]) | ((MS_2BYTES)(data[i]))<<8;
	}

	inline
	MS_4BYTES Byte::Get3Bytes(const MS_BYTE* data, size_t i)
	{
		return (MS_4BYTES)(data[i+2]) | ((MS_4BYTES)(data[i+1]))<<8 | ((MS_4BYTES)(data[i]))<<16;
	}

	inline
	MS_4BYTES Byte::Get4Bytes(const MS_BYTE* data, size_t i)
	{
		return (MS_4BYTES)(data[i+3]) | ((MS_4BYTES)(data[i+2]))<<8 | ((MS_4BYTES)(data[i+1]))<<16 | ((MS_4BYTES)(data[i]))<<24;
	}

	inline
	MS_8BYTES Byte::Get8Bytes(const MS_BYTE* data, size_t i)
	{
		return ((MS_8BYTES)Byte::Get4Bytes(data,i))<<32 | Byte::Get4Bytes(data,i+4);
	}

	inline
	void Byte::Set1Byte(MS_BYTE* data, size_t i, MS_BYTE value)
	{
		data[i] = value;
	}

	inline
	void Byte::Set2Bytes(MS_BYTE* data, size_t i, MS_2BYTES value)
	{
		data[i+1] = (MS_BYTE)(value);
		data[i]   = (MS_BYTE)(value>>8);
	}

	inline
	void Byte::Set3Bytes(MS_BYTE* data, size_t i, MS_4BYTES value)
	{
		data[i+2] = (MS_BYTE)(value);
		data[i+1] = (MS_BYTE)(value>>8);
		data[i]   = (MS_BYTE)(value>>16);
	}

	inline
	void Byte::Set4Bytes(MS_BYTE* data, size_t i, MS_4BYTES value)
	{
		data[i+3] = (MS_BYTE)(value);
		data[i+2] = (MS_BYTE)(value>>8);
		data[i+1] = (MS_BYTE)(value>>16);
		data[i]   = (MS_BYTE)(value>>24);
	}

	inline
	void Byte::Set8Bytes(MS_BYTE* data, size_t i, MS_8BYTES value)
	{
		data[i+7] = (MS_BYTE)(value);
		data[i+6] = (MS_BYTE)(value>>8);
		data[i+5] = (MS_BYTE)(value>>16);
		data[i+4] = (MS_BYTE)(value>>24);
		data[i+3] = (MS_BYTE)(value>>32);
		data[i+2] = (MS_BYTE)(value>>40);
		data[i+1] = (MS_BYTE)(value>>48);
		data[i]   = (MS_BYTE)(value>>56);
	}

	inline
	MS_2BYTES Byte::PadTo4Bytes(MS_2BYTES size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFC) + 4;
		else
			return size;
	}

	inline
	MS_4BYTES Byte::PadTo4Bytes(MS_4BYTES size)
	{
		// If size is not multiple of 32 bits then pad it.
		if (size & 0x03)
			return (size & 0xFFFFFFFC) + 4;
		else
			return size;
	}

	class Crypto
	{
	public:
		static void ThreadInit();
		static void ThreadDestroy();
		static unsigned int GetRandomUInt(unsigned int min, unsigned int max);
		static const char* GetRandomHexString(char* str, size_t len);
		static MS_4BYTES CRC32(const MS_BYTE* data, size_t size);
		static const MS_BYTE* HMAC_SHA1(const std::string &key, const MS_BYTE* data, size_t len);

	private:
		static __thread unsigned int seed;
		static __thread HMAC_CTX hmacSha1Ctx;
		static __thread MS_BYTE hmacSha1Buffer[];
		static const MS_4BYTES crc32Table[256];
	};

	/* Inline static methods. */

	inline
	unsigned int Crypto::GetRandomUInt(unsigned int min, unsigned int max)
	{
		// NOTE: This is the original, but produces very small values.
		// Crypto::seed = (214013 * Crypto::seed) + 2531011;
		// return (((Crypto::seed>>16)&0x7FFF) % (max - min + 1)) + min;

		// This seems to produce better results.
		Crypto::seed = (unsigned int)((214013 * Crypto::seed) + 2531011);
		return (((Crypto::seed>>4)&0x7FFF7FFF) % (max - min + 1)) + min;
	}

	inline
	const char* Crypto::GetRandomHexString(char* buffer, size_t len)
	{
		static const char hex_chars[] =	{
			'0','1','2','3','4','5','6','7','8','9',
			'A','B','C','D','E','F','G','H','I','J',
			'K','L','M','N','O','P','Q','R','S','T',
			'U','V','W','X','Y','Z'
		};

		for (size_t i = 0; i < len; ++i)
			buffer[i] = hex_chars[GetRandomUInt(0, sizeof(hex_chars) - 1)];

		return buffer;
	}

	inline
	MS_4BYTES Crypto::CRC32(const MS_BYTE* data, size_t size)
	{
		MS_4BYTES crc = 0xFFFFFFFF;
		const MS_BYTE* p = data;

		while (size--)
			crc = Crypto::crc32Table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

		return crc ^ ~0U;
	}
}  // namespace Utils

#endif
