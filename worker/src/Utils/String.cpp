/*
 * Code from http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
 *
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README_BASE64_UTILS for more details.
 */

#define MS_CLASS "Utils::String"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

/* Static. */

static constexpr size_t BufferOutSize{ 65536 };
static thread_local uint8_t BufferOut[BufferOutSize];
static constexpr uint8_t Base64Table[65] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace Utils
{
	std::string String::Base64Encode(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		uint8_t* const out       = BufferOut;
		const size_t olen        = (len * 4 / 3) + 4; // 3-byte blocks to 4-byte.
		const uint8_t* const end = data + len;

		const uint8_t* in = data;
		// NOTE: misc-const-correctness does not take `*pos++ = x` as a write through
		// `pos`, so it wrongly suggests making its pointee const.
		// NOLINTNEXTLINE(misc-const-correctness)
		uint8_t* pos = out;

		if (olen < len)
		{
			MS_THROW_TYPE_ERROR("integer overflow");
		}
		else if (olen > BufferOutSize - 1)
		{
			MS_THROW_TYPE_ERROR("data too big");
		}

		while (end - in >= 3)
		{
			const uint32_t byte0 = in[0];
			const uint32_t byte1 = in[1];
			const uint32_t byte2 = in[2];

			*pos++ = Base64Table[byte0 >> 2];
			*pos++ = Base64Table[((byte0 & 0x03) << 4) | (byte1 >> 4)];
			*pos++ = Base64Table[((byte1 & 0x0f) << 2) | (byte2 >> 6)];
			*pos++ = Base64Table[byte2 & 0x3f];
			in += 3;
		}

		if (end - in)
		{
			const uint32_t byte0 = in[0];

			*pos++ = Base64Table[byte0 >> 2];

			if (end - in == 1)
			{
				*pos++ = Base64Table[(byte0 & 0x03) << 4];
				*pos++ = '=';
			}
			else
			{
				const uint32_t byte1 = in[1];

				*pos++ = Base64Table[((byte0 & 0x03) << 4) | (byte1 >> 4)];
				*pos++ = Base64Table[(byte1 & 0x0f) << 2];
			}

			*pos++ = '=';
		}

		return { reinterpret_cast<const char*>(out), static_cast<size_t>(pos - out) };
	}

	std::string String::Base64Encode(const std::string& str)
	{
		MS_TRACE();

		const auto* data = reinterpret_cast<const uint8_t*>(str.c_str());

		return Base64Encode(data, str.size());
	}

	uint8_t* String::Base64Decode(const uint8_t* data, size_t len, size_t& outLen)
	{
		MS_TRACE();

		uint8_t* const out = BufferOut;

		uint8_t dtable[256];
		uint8_t* pos;
		uint32_t block[4];
		uint8_t tmp;
		size_t i;
		size_t count;
		size_t pad{ 0 };

		// NOTE: This is not really accurate but anyway.
		if (len > BufferOutSize - 1)
		{
			MS_THROW_TYPE_ERROR("data too big");
		}

		std::memset(dtable, 0x80, 256);

		for (i = 0; i < sizeof(Base64Table) - 1; ++i)
		{
			dtable[Base64Table[i]] = static_cast<uint8_t>(i);
		}

		dtable[uint8_t{ '=' }] = 0;

		count = 0;

		for (i = 0; i < len; ++i)
		{
			if (dtable[data[i]] != 0x80)
			{
				count++;
			}
		}

		if (count == 0 || count % 4)
		{
			MS_THROW_TYPE_ERROR("invalid data");
		}

		pos   = out;
		count = 0;

		for (i = 0; i < len; ++i)
		{
			tmp = dtable[data[i]];

			if (tmp == 0x80)
			{
				continue;
			}

			if (data[i] == '=')
			{
				pad++;
			}

			block[count] = tmp;
			count++;

			if (count == 4)
			{
				*pos++ = (block[0] << 2) | (block[1] >> 4);
				*pos++ = (block[1] << 4) | (block[2] >> 2);
				*pos++ = (block[2] << 6) | block[3];
				count  = 0;

				if (pad)
				{
					if (pad == 1)
					{
						pos--;
					}
					else if (pad == 2)
					{
						pos -= 2;
					}
					else
					{
						MS_THROW_TYPE_ERROR("integer padding");
					}

					break;
				}
			}
		}

		outLen = pos - out;

		return out;
	}

	uint8_t* String::Base64Decode(const std::string& str, size_t& outLen)
	{
		MS_TRACE();

		const auto* data = reinterpret_cast<const uint8_t*>(str.c_str());

		return Base64Decode(data, str.size(), outLen);
	}
} // namespace Utils
