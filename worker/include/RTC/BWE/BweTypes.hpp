#ifndef MS_RTC_BWE_BWE_TYPES_HPP
#define MS_RTC_BWE_BWE_TYPES_HPP

#include "common.hpp"
#include <string_view>

namespace RTC
{
	namespace BWE
	{
		namespace Types
		{
			/**
			 * How the network is behaving according to the delay based detector.
			 */
			enum class BandwidthUsage : uint8_t
			{
				/**
				 * Queues are not growing.
				 */
				NORMAL,
				/**
				 * Queues are being emptied, so we are sending below the link capacity.
				 */
				UNDERUSING,
				/**
				 * Queues are filling up, so we are sending above the link capacity.
				 */
				OVERUSING
			};

			constexpr std::string_view BandwidthUsageToString(BandwidthUsage bandwidthUsage)
			{
				switch (bandwidthUsage)
				{
					case BandwidthUsage::NORMAL:
					{
						return "NORMAL";
					}

					case BandwidthUsage::UNDERUSING:
					{
						return "UNDERUSING";
					}

					case BandwidthUsage::OVERUSING:
					{
						return "OVERUSING";
					}

						NO_DEFAULT_GCC();
				}
			}
		} // namespace Types
	} // namespace BWE
} // namespace RTC

#endif
