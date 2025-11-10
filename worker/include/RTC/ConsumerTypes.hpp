#ifndef MS_RTC_CONSUMER_TYPES_HPP
#define MS_RTC_CONSUMER_TYPES_HPP

#include "common.hpp"

namespace RTC
{
	namespace ConsumerTypes
	{
		struct VideoLayers
		{
			int16_t spatial{ -1 };
			int16_t temporal{ -1 };

			VideoLayers() = default;

			VideoLayers(int16_t spatial, int16_t temporal) : spatial(spatial), temporal(temporal)
			{
			}

			VideoLayers(const VideoLayers& other) = default;
			bool operator==(const VideoLayers& other) const
			{
				return spatial == other.spatial && temporal == other.temporal;
			}

			bool operator!=(const VideoLayers& other) const
			{
				return !(*this == other);
			}

			void Reset()
			{
				spatial  = -1;
				temporal = -1;
			}
		};
	} // namespace ConsumerTypes
} // namespace RTC

#endif
