#define MS_CLASS "Utils::Bits"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	size_t Bits::CountSetBits(uint16_t mask)
	{
		MS_TRACE();

		return static_cast<size_t>(__builtin_popcount(mask));
	}

	uint8_t Bits::GetBitAt(const uint8_t* data, size_t len, uint32_t offset)
	{
		MS_TRACE();

		const size_t byteOffset = offset >> 3;

		MS_ASSERT(
		  byteOffset < len, "given offset is past the data [offset:%" PRIu32 ", len:%zu]", offset, len);

		return (data[byteOffset] >> (7 - (offset & 0x7))) & 0x1;
	}
} // namespace Utils
