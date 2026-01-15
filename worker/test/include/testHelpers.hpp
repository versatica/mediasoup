#ifndef MS_TEST_HELPERS_HPP
#define MS_TEST_HELPERS_HPP

#include "common.hpp"

namespace helpers
{
	bool ReadBinaryFile(const char* file, uint8_t* buffer, size_t* len);

	bool AddToBuffer(uint8_t* buf, size_t* size, const uint8_t* data, size_t len);

	bool ReadPayloadData(const char* file, int pos, int bytes, uint8_t* payload);

	bool AreBuffersEqual(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2);
} // namespace helpers

#endif
