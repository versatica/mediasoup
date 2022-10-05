#ifndef MS_FUZZER_RTC_RTP_STREAM_SEND_HPP
#define MS_FUZZER_RTC_RTP_STREAM_SEND_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RtpStreamSend
		{
			void Fuzz(const uint8_t* data, size_t len);
		}
	} // namespace RTC
} // namespace Fuzzer

#endif
