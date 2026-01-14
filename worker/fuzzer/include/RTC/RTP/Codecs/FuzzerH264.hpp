#ifndef MS_FUZZER_RTC_RTP_CODECS_H264_HPP
#define MS_FUZZER_RTC_RTP_CODECS_H264_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTP
		{
			namespace Codecs
			{
				namespace H264
				{
					void Fuzz(const uint8_t* data, size_t len);
				}
			} // namespace Codecs
		} // namespace RTP
	} // namespace RTC
} // namespace Fuzzer

#endif
