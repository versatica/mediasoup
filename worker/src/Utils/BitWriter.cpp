#define MS_CLASS "Utils::BitWriter"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	BitWriter::BitWriter(uint8_t* data, size_t len) : data(data), len(len)
	{
		MS_TRACE();
	}

	uint32_t BitWriter::GetOffset() const
	{
		MS_TRACE();

		return this->offset;
	}

	void BitWriter::PutBit(uint8_t bit)
	{
		MS_TRACE();

		const size_t byteOffset = this->offset >> 3;

		MS_ASSERT(
		  byteOffset < this->len,
		  "given offset is past the buffer [offset:%" PRIu32 ", len:%zu]",
		  this->offset,
		  this->len);

		if (bit)
		{
			this->data[byteOffset] |= (1U << (7 - (this->offset & 0x7)));
		}

		++this->offset;
	}

	void BitWriter::PutBits(uint32_t count, uint32_t bits)
	{
		MS_TRACE();

		for (uint32_t i{ 0 }; i < count; ++i)
		{
			PutBit((bits >> (count - i - 1)) & 0x1);
		}
	}

	void BitWriter::PutBitRange(const uint8_t* src, size_t len, uint32_t from, uint32_t to)
	{
		MS_TRACE();

		// NOTE: An empty range is fine, since the fields being copied over are
		// optional and absent ones delimit nothing.
		MS_ASSERT(from <= to, "from is higher than to [from:%" PRIu32 ", to:%" PRIu32 "]", from, to);

		for (uint32_t i{ from }; i < to; ++i)
		{
			PutBit(Bits::GetBitAt(src, len, i));
		}
	}
} // namespace Utils
