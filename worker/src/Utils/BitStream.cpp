#define MS_CLASS "Utils::BitStream"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	//NOLINTNEXTLINE (cppcoreguidelines-pro-type-member-init)
	BitStream::BitStream(uint8_t* data, size_t len) : len(len)
	{
		MS_TRACE();

		std::memcpy(this->data, data, len);
	}

	const uint8_t* BitStream::GetData() const
	{
		MS_TRACE();

		return this->data;
	}

	size_t BitStream::GetLength() const
	{
		MS_TRACE();

		return this->len;
	}

	uint32_t BitStream::GetOffset() const
	{
		MS_TRACE();

		return this->offset;
	}

	void BitStream::Reset()
	{
		MS_TRACE();

		this->offset = 0;
		this->len    = sizeof(this->data);

		std::memset(this->data, 0, this->len);
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
			bits = (2 * bits) + GetBit();
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

		auto leftBits = (this->len * 8) - this->offset;

		return leftBits;
	}

	void BitStream::SkipBits(size_t count)
	{
		MS_TRACE();

		this->offset += count;
	}

	/*
	 * non-symmetric unsigned encoded integer with maximum
	 * number of values n (i.e., output in range 0..n-1).
	 * Returns std::nullopt if there are not enough bits left.
	 */
	std::optional<uint32_t> BitStream::ReadNs(uint32_t n)
	{
		unsigned w = 0;
		unsigned x = n;

		while (x != 0)
		{
			x = x >> 1;
			++w;
		}

		if (this->GetLeftBits() < w - 1)
		{
			return std::nullopt;
		}

		const unsigned v = this->GetBits(w - 1);
		const unsigned m = (1u << w) - n;

		if (v < m)
		{
			return v;
		}

		if (this->GetLeftBits() < 1)
		{
			return std::nullopt;
		}

		const unsigned extraBit = this->GetBit();

		return (v << 1) - m + extraBit;
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

		const unsigned m = (1 << w) - n;

		if (v < m)
		{
			this->PutBits(offset, w - 1, v);
		}
		else
		{
			this->PutBits(offset, w, v + m);
		}
	}

	void BitStream::PutBit(uint8_t bit)
	{
		MS_TRACE();

		this->PutBit(this->offset, bit);
	}

	void BitStream::PutBits(uint32_t count, uint32_t bits)
	{
		MS_TRACE();

		this->PutBits(this->offset, count, bits);
	}

	void BitStream::PutBit(uint32_t offset, uint8_t bit)
	{
		MS_TRACE();

		// Retrieve the current byte position.
		const size_t byteOffset = offset >> 0x3;

		// Calculate the bitmask for the target bit within the current byte.
		const auto bitmask = (1u << (0x7 - (offset & 0x7)));

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
			const uint32_t shift = count - i - 1;
			const uint8_t bit    = (bits >> shift) & 0x1;

			this->PutBit(offset++, bit);
		}
	}
} // namespace Utils
