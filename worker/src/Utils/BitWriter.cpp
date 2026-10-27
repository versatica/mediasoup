#define MS_CLASS "Utils::BitWriter"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	BitWriter::BitWriter(uint8_t* data) : data(data)
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

		if (bit)
		{
			this->data[this->offset >> 3] |= (1U << (7 - (this->offset & 0x7)));
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

	void BitWriter::CopyBits(const uint8_t* src, uint32_t from, uint32_t to)
	{
		MS_TRACE();

		for (uint32_t i{ from }; i < to; ++i)
		{
			PutBit(Bits::GetBitAt(src, i));
		}
	}
} // namespace Utils
