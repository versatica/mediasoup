#ifndef MS_RTC_CODECS_VP8_HPP
#define MS_RTC_CODECS_VP8_HPP

#include "common.hpp"

namespace RTC
{
	namespace Codecs
	{
		class VP8
		{
		public:
			static bool IsKeyFrame(const uint8_t* data, size_t len);
		};
	} // namespace Codecs
} // namespace RTC

#endif
