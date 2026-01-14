#ifndef MS_FUZZER_RTC_RTP_CODECS_AV1_HPP
#define MS_FUZZER_RTC_RTP_CODECS_AV1_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTP
		{
			namespace Codecs
			{
				namespace AV1
				{
					void Fuzz(const uint8_t* data, size_t len);
				} // namespace AV1
			} // namespace Codecs
		} // namespace RTP
	} // namespace RTC
} // namespace Fuzzer

#endif
