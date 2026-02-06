#ifndef MS_FUZZER_UTILS_HPP
#define MS_FUZZER_UTILS_HPP

#include "common.hpp"

namespace FuzzerUtils
{
	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerUtils

#endif
