#define MS_CLASS "Utils::Number"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"

namespace Utils
{
	uint64_t Number::ForwardDiff(uint64_t a, uint64_t b, uint64_t max)
	{
		MS_TRACE();

		MS_ASSERT(a < max, "a must be lower than max");
		MS_ASSERT(b < max, "b must be lower than max");

		return a <= b ? b - a : max - (a - b);
	}

	uint64_t Number::ReverseDiff(uint64_t a, uint64_t b, uint64_t max)
	{
		MS_TRACE();

		MS_ASSERT(a < max, "a must be lower than max");
		MS_ASSERT(b < max, "b must be lower than max");

		return b <= a ? a - b : max - (b - a);
	}
} // namespace Utils
