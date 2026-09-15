#ifndef MS_RTC_BWE_UTILS_HPP
#define MS_RTC_BWE_UTILS_HPP

#include "common.hpp"

namespace RTC
{
	namespace BWE
	{
		class Utils
		{
		public:
			/**
			 * Bitrate that results from applying a factor to another one (bps).
			 *
			 * @remarks
			 * - The algorithm derives a bitrate from another one this way all over the
			 *   place, and the result has to come back to an int64_t. A value beyond
			 *   what that type holds becomes `Types::BitrateInfinite` rather than the
			 *   undefined one that converting it would give.
			 */
			static int64_t ApplyBitrateFactor(int64_t bitrate, double factor);

			/**
			 * Sum of two bitrates (bps).
			 *
			 * @remarks
			 * - A sum beyond what an int64_t can hold becomes `Types::BitrateInfinite`
			 *   rather than wrapping around into a negative bitrate.
			 */
			static int64_t AddBitrates(int64_t bitrate, int64_t otherBitrate);
		};
	} // namespace BWE
} // namespace RTC

#endif
