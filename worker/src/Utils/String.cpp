#define MS_CLASS "Utils::String"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

static const char* B64Chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// clang-format off
static const int B64Index[256] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
	0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
	0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};
// clang-format on

namespace Utils
{
	const std::string Utils::String::Base64Encode(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		std::string result((len + 2) / 3 * 4, '=');
		const char* p = reinterpret_cast<const char*>(data);
		char* str = &result[0];
		size_t j{ 0 };
		size_t pad = len % 3;
		const size_t last = len - pad;

		for (size_t i{ 0 }; i < last; i += 3)
		{
			int n = static_cast<int>(p[i]) << 16 | static_cast<int>(p[i + 1]) << 8 | p[i + 2];

			str[j++] = B64Chars[n >> 18];
			str[j++] = B64Chars[n >> 12 & 0x3F];
			str[j++] = B64Chars[n >> 6 & 0x3F];
			str[j++] = B64Chars[n & 0x3F];
		}

		if (pad)
		{
			int n = --pad ? static_cast<int>(p[last]) << 8 | p[last + 1] : p[last];

			str[j++] = B64Chars[pad ? n >> 10 & 0x3F : n >> 2];
			str[j++] = B64Chars[pad ? n >> 4 & 0x03F : n << 4 & 0x3F];
			str[j++] = pad ? B64Chars[n << 2 & 0x3F] : '=';
		}

		return result;
	}

	const std::string Utils::String::Base64Encode(std::string& data)
	{
		MS_TRACE();

		return Base64Encode(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
	}

	const uint8_t* Utils::String::Base64Decode(const std::string& str)
	{
		MS_TRACE();

		const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());
		const size_t len = str.size();

		if (len == 0)
			return nullptr;

		const unsigned char* p = static_cast<const unsigned char*>(data);
		size_t j{ 0 };
		size_t pad1 = len % 4 || p[len - 1] == '=';
		size_t pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
		const size_t last = (len - pad1) / 4 << 2;
		std::string result(last / 4 * 3 + pad1 + pad2, '\0');
		// const unsigned char* out = static_cast<const unsigned char*>(&result[0]);
		char* out = &result[0];

		for (size_t i = 0; i < last; i += 4)
		{
			int n = B64Index[p[i]] << 18 | B64Index[p[i + 1]] << 12 | B64Index[p[i + 2]] << 6 | B64Index[p[i + 3]];

			out[j++] = n >> 16;
			out[j++] = n >> 8 & 0xFF;
			out[j++] = n & 0xFF;
		}

		if (pad1)
		{
			int n = B64Index[p[last]] << 18 | B64Index[p[last + 1]] << 12;

			out[j++] = n >> 16;

			if (pad2)
			{
				n |= B64Index[p[last + 2]] << 6;
				out[j++] = n >> 8 & 0xFF;
			}
		}

		return reinterpret_cast<const uint8_t*>(result.c_str());
	}
} // namespace Utils
