#ifndef MS_TEST_HELPERS_HPP
#define MS_TEST_HELPERS_HPP

#include "common.hpp"

namespace helpers
{
	bool readBinaryFile(const char* file, uint8_t* buffer, size_t* len);

	bool areBuffersEqual(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2);
} // namespace helpers

#endif
