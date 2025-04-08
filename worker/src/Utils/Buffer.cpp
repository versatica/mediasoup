#define MS_CLASS "Utils::Buffer"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy(), std::memmove()

namespace Utils
{
	bool Utils::Buffer::DoBuffersOverlap(const uint8_t* dstBuffer, const uint8_t* buffer, size_t length)
	{
		MS_TRACE();

		// clang-format off
		return (
			(dstBuffer == buffer) ||
			(dstBuffer > buffer && dstBuffer - buffer < length) ||
			(dstBuffer < buffer && buffer - dstBuffer < length)
		);
		// clang-format on
	}

	bool Utils::Buffer::MemcpyOrMemmove(uint8_t* dstBuffer, const uint8_t* buffer, size_t length)
	{
		MS_TRACE();

		if (DoBuffersOverlap(dstBuffer, buffer, length))
		{
			std::memmove(dstBuffer, buffer, length);

			return true;
		}
		else
		{
			std::memcpy(dstBuffer, buffer, length);

			return false;
		}
	}
} // namespace Utils
