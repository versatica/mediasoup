#define MS_CLASS "Utils::BitReader"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
	BitReader::BitReader(const uint8_t* data, size_t len) : len(len)
	{
		MS_TRACE();

		// NOTE: Whatever comes from the network is the value of an RTP header
		// extension, whose length field is a single byte, so it always fits. This
		// only catches a caller handing over a longer buffer.
		MS_ASSERT(
		  len <= sizeof(this->data),
		  "given length exceeds the buffer [len:%zu, buffer:%zu]",
		  len,
		  sizeof(this->data));

		std::memcpy(this->data, data, len);
	}

	const uint8_t* BitReader::GetData() const
	{
		MS_TRACE();

		return this->data;
	}

	size_t BitReader::GetLength() const
	{
		MS_TRACE();

		return this->len;
	}

	uint32_t BitReader::GetOffset() const
	{
		MS_TRACE();

		return this->offset;
	}

	uint8_t BitReader::GetBit()
	{
		MS_TRACE();

		// There is nothing left to read, so stay where the data ended rather than
		// read past it. Callers are expected to check GetLeftBits() themselves, and
		// a zero cannot be told apart from a real one, so this is the last resort
		// and not the way to know that the data is over.
		if (GetLeftBits() == 0)
		{
			return 0;
		}

		auto bit = ((*(data + (this->offset >> 0x3))) >> (0x7 - (this->offset & 0x7))) & 0x1;

		this->offset++;

		return bit;
	}

	uint32_t BitReader::GetBits(size_t count)
	{
		MS_TRACE();

		uint32_t bits = 0;

		for (unsigned i = 0; i < count; ++i)
		{
			bits = (2 * bits) + GetBit();
		}

		return bits;
	}

	uint32_t BitReader::GetLeftBits() const
	{
		MS_TRACE();

		if (this->offset >= this->len * 8)
		{
			return 0;
		}

		auto leftBits = (this->len * 8) - this->offset;

		return leftBits;
	}

	void BitReader::SkipBits(size_t count)
	{
		MS_TRACE();

		this->offset += count;
	}

	/*
	 * non-symmetric unsigned encoded integer with maximum
	 * number of values n (i.e., output in range 0..n-1).
	 * Returns std::nullopt if there are not enough bits left.
	 */
	std::optional<uint32_t> BitReader::ReadNs(uint32_t n)
	{
		MS_TRACE();

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
} // namespace Utils
