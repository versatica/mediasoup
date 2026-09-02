#ifndef MS_RTC_BWE_BWE_TYPES_HPP
#define MS_RTC_BWE_BWE_TYPES_HPP

#include "common.hpp"
#include <limits>
#include <string_view>

namespace RTC
{
	namespace BWE
	{
		namespace Types
		{
			/**
			 * Bitrate meaning that there is no limit at all (bps).
			 */
			constexpr int64_t BitrateInfinite{ std::numeric_limits<int64_t>::max() };

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

			/**
			 * Estimate of the capacity of the link, used to bound the rate control.
			 */
			struct NetworkStateEstimate
			{
				/**
				 * Safe measure of the available capacity (bps).
				 */
				std::optional<int64_t> linkCapacityLower;
				/**
				 * Limit applied when increasing the bitrate (bps).
				 */
				std::optional<int64_t> linkCapacityUpper;
			};

			/**
			 * Input given to the rate control on every update.
			 */
			struct RateControlInput
			{
				/**
				 * How the network is behaving according to the delay based detector.
				 */
				BandwidthUsage bandwidthUsage;
				/**
				 * Bitrate acknowledged by the receiver (bps), or no value if it could
				 * not be measured.
				 */
				std::optional<int64_t> estimatedThroughput;
			};
		} // namespace Types
	} // namespace BWE
} // namespace RTC

#endif
