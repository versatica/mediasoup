#define MS_CLASS "RTC::ConsumerTypes"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/ConsumerTypes.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace ConsumerTypes
	{
		void VideoLayers::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<ConsumerTypes::VideoLayers>");
			MS_DUMP_CLEAN(
			  indentation, "  spatial:%" PRIi16 ", temporal:%" PRIi16, this->spatial, this->temporal);
			MS_DUMP_CLEAN(indentation, "</ConsumerTypes::VideoLayers>");
		}
	} // namespace ConsumerTypes
} // namespace RTC
