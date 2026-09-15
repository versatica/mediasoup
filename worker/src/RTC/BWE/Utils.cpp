#define MS_CLASS "RTC::BWE::Utils"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/Utils.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BweTypes.hpp"

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

			return static_cast<int64_t>(result);
		}
	} // namespace BWE
} // namespace RTC
