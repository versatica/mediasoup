/*
 * Code from http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c.
 *
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#define MS_CLASS "Utils::String"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring> // std::memset()

/* Static. */

static constexpr size_t BufferOutSize{ 65536 };
static unsigned char BufferOut[BufferOutSize];
static const unsigned char Base64Table[65] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace Utils
{
	/**
	 * @param data - Data to be encoded.
	 * @param len - Length of the data to be encoded.
	 * @param outLen - Pointer to output length variable.
	 *
	 * @returns Buffer of outLen bytes of encoded data, or nulllptr on failure.
	 *
	 * Returned buffer is nul terminated to make it easier to use as a C string.
	 * The nul terminator is not included in outLen.
	 */
	unsigned char* Utils::String::Base64Encode(const unsigned char* data, size_t len, size_t* outLen)
	{
		MS_TRACE();

		unsigned char* out = BufferOut;
		unsigned char* pos;
		const unsigned char* end;
		const unsigned char* in;
		size_t olen;
		int lineLen;

		olen = len * 4 / 3 + 4; // 3-byte blocks to 4-byte.
		olen += olen / 72;      // line feeds.
		olen++;                 // Nul termination.

		if (olen < len)
			return nullptr; // Integer overflow.

		end     = data + len;
		in      = data;
		pos     = out;
		lineLen = 0;

		while (end - in >= 3)
		{
			*pos++ = Base64Table[in[0] >> 2];
			*pos++ = Base64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = Base64Table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
			*pos++ = Base64Table[in[2] & 0x3f];
			in += 3;
			lineLen += 4;

			if (lineLen >= 72)
			{
				*pos++  = '\n';
				lineLen = 0;
			}
		}

		if (end - in)
		{
			*pos++ = Base64Table[in[0] >> 2];

			if (end - in == 1)
			{
				*pos++ = Base64Table[(in[0] & 0x03) << 4];
				*pos++ = '=';
			}
			else
			{
				*pos++ = Base64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
				*pos++ = Base64Table[(in[1] & 0x0f) << 2];
			}

			*pos++ = '=';
			lineLen += 4;
		}

		if (lineLen)
			*pos++ = '\n';

		*pos = '\0';

		if (outLen)
			*outLen = pos - out;

		return out;
	}

	unsigned char* Utils::String::Base64Encode(const std::string& data, size_t* outLen)
	{
		MS_TRACE();

		return Base64Encode(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), outLen);
	}

	/**
	 * @param data - Data to be decoded.
	 * @param len - Length of the data to be decoded.
	 * @param outLen - Pointer to output length variable.
	 *
	 * @returns Buffer of outLen bytes of decoded data, or nullptr on failure.
	 */
	unsigned char* Utils::String::Base64Decode(const unsigned char* data, size_t len, size_t* outLen)
	{
		MS_TRACE();

		unsigned char dtable[256];
		unsigned char* out = BufferOut;
		unsigned char* pos;
		unsigned char block[4];
		unsigned char tmp;
		size_t i;
		size_t count;
		size_t olen;
		int pad{ 0 };

		std::memset(dtable, 0x80, 256);

		for (i = 0; i < sizeof(Base64Table) - 1; ++i)
		{
			dtable[Base64Table[i]] = static_cast<unsigned char>(i);
		}
		dtable[static_cast<unsigned char>('=')] = 0;

		count = 0;
		for (i = 0; i < len; ++i)
		{
			if (dtable[data[i]] != 0x80)
				count++;
		}

		if (count == 0 || count % 4)
			return nullptr;

		olen  = count / 4 * 3;
		pos   = out;
		count = 0;

		for (i = 0; i < len; ++i)
		{
			tmp = dtable[data[i]];

			if (tmp == 0x80)
				continue;

			if (data[i] == '=')
				pad++;

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
						pos--;
					else if (pad == 2)
						pos -= 2;
					else
					{
						// Invalid padding.
						return nullptr;
					}

					break;
				}
			}
		}

		*outLen = pos - out;

		return out;
	}

	unsigned char* Utils::String::Base64Decode(const std::string& data, size_t* outLen)
	{
		MS_TRACE();

		return Base64Decode(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), outLen);
	}
} // namespace Utils
