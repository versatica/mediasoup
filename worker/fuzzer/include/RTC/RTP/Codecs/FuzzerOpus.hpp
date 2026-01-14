#ifndef MS_FUZZER_RTC_RTP_CODECS_OPUS_HPP
#define MS_FUZZER_RTC_RTP_CODECS_OPUS_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTP
		{
			namespace Codecs
			{
				namespace Opus
				{
					void Fuzz(const uint8_t* data, size_t len);
				}
			} // namespace Codecs
		} // namespace RTP
	} // namespace RTC
} // namespace Fuzzer

#endif
