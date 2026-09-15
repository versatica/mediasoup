#define MS_CLASS "RTC::BWE::Utils"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/Utils.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <cmath> // std::llround()

namespace RTC
{
	namespace BWE
	{
		/* Class methods. */

		int64_t Utils::ApplyBitrateFactor(int64_t bitrate, double factor)
		{
			MS_TRACE();

			const double result = static_cast<double>(bitrate) * factor;

			// NOTE: Converting a value that the type cannot hold is undefined
			// behaviour, and the maximum bitrate is chosen by the application, so
			// nothing keeps this product within range.
			if (result >= static_cast<double>(Types::BitrateInfinite))
			{
				return Types::BitrateInfinite;
			}

			if (result <= 0.0)
			{
				return 0;
			}

			// The result is rounded rather than truncated, since a factor is applied
			// over and over to a bitrate that keeps being derived from the previous
			// one and always losing the fraction drifts it downwards.
			return std::llround(result);
		}

		int64_t Utils::AddBitrates(int64_t bitrate, int64_t otherBitrate)
		{
			MS_TRACE();

			// NOTE: Signed overflow is undefined behaviour, and both operands can be
			// as high as the application allows.
			if (bitrate > Types::BitrateInfinite - otherBitrate)
			{
				return Types::BitrateInfinite;
			}

			return bitrate + otherBitrate;
		}
	} // namespace BWE
} // namespace RTC
