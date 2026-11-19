#ifndef MS_TEST_HELPERS_HPP
#define MS_TEST_HELPERS_HPP

#include "common.hpp"

namespace helpers
{
	/**
	 * Fills the buffer with bytes that change from one position to the next,
	 * starting at `firstByte`, so no two consecutive bytes are equal.
	 */
	void FillBuffer(uint8_t* buffer, size_t bufferLength, uint8_t firstByte);

	[[nodiscard]] bool ReadBinaryFile(const char* file, uint8_t* buffer, size_t* len);

	[[nodiscard]] bool AreBuffersEqual(
	  const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2);
} // namespace helpers

#endif
