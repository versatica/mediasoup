#define MS_CLASS "Utils::BitStream"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	BitStream::BitStream(uint8_t* data, size_t len) : data(data), len(len)
	{
		MS_TRACE();
	}

	uint8_t BitStream::GetBit()
	{
		MS_TRACE();

		auto bit = ((*(data + (this->offset >> 0x3))) >> (0x7 - (this->offset & 0x7))) & 0x1;

		this->offset++;

		return bit;
	}

	uint32_t BitStream::GetBits(size_t count)
	{
		MS_TRACE();

		uint32_t bits = 0;

		for (unsigned i = 0; i < count; ++i)
		{
			bits = 2 * bits + GetBit();
		}

		return bits;
	}

	uint32_t BitStream::GetLeftBits() const
	{
		MS_TRACE();

		if (this->offset >= this->len * 8)
		{
			return 0;
		}

		auto leftBits = this->len * 8 - this->offset;

		return leftBits;
	}

	void BitStream::SkipBits(size_t count)
	{
		MS_TRACE();

		this->offset += count;
	}

	void BitStream::Write(uint32_t offset, uint32_t n, uint32_t v)
	{
		MS_TRACE();

		unsigned w = 0;
		unsigned x = n;

		while (x != 0)
		{
			x = x >> 1;
			++w;
		}

		unsigned m = (1 << w) - n;

		if (v < m)
		{
			this->PutBits(offset, w - 1, v);
		}
		else
		{
			this->PutBits(offset, w, v + m);
		}
	}

	void BitStream::PutBit(uint32_t offset, uint8_t bit)
	{
		MS_TRACE();

		// Retrieve the current byte position.
		size_t byteOffset = offset >> 0x3;

		// Calculate the bitmask for the target bit within the current byte.
		auto bitmask = (1u << (0x7 - (offset & 0x7)));

		if (bit)
		{
			this->data[byteOffset] |= bitmask;
		}
		else
		{
			this->data[byteOffset] &= ~bitmask;
		}

		++this->offset;
	}

	void BitStream::PutBits(uint32_t offset, uint32_t count, uint32_t bits)
	{
		MS_TRACE();

		MS_ASSERT(
		  count <= 32, "count cannot be bigger than 32 [count:%" PRIu32 ", bits:%" PRIu32 "]", count, bits);

		for (unsigned i = 0; i < count; ++i)
		{
			uint32_t shift = count - i - 1;
			uint8_t bit    = (bits >> shift) & 0x1;

			this->PutBit(offset++, bit);
		}
	}
} // namespace Utils
