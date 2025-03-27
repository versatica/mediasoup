#define MS_CLASS "RTC::Codecs::BitStream"

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	BitStream::BitStream(const uint8_t* data, size_t len) : data(data), len(len)
	{
	}

	uint8_t BitStream::GetBit()
	{
		auto bit = ((*(data + (this->offset >> 0x3))) >> (0x7 - (this->offset & 0x7))) & 0x1;

		this->offset++;

		return bit;
	}

	uint32_t BitStream::GetBits(size_t count)
	{
		uint32_t bits = 0;

		for (unsigned i = 0; i < count; i++)
		{
			bits = 2 * bits + GetBit();
		}

		return bits;
	}

	uint32_t BitStream::GetLeftBits() const
	{
		if (this->offset >= this->len * 8)
		{
			return 0;
		}

		auto leftBits = this->len * 8 - this->offset;

		return leftBits;
	}

	void BitStream::SkipBits(size_t count)
	{
		this->offset += count;
	}
} // namespace Utils
  // namespace RTC
