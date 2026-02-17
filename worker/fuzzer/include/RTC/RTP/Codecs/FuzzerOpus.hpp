#ifndef MS_FUZZER_RTC_RTP_CODECS_OPUS_HPP
#define MS_FUZZER_RTC_RTP_CODECS_OPUS_HPP

#include "common.hpp"

namespace FuzzerRtcRtpCodecsOpus
{
	void Fuzz(const uint8_t* data, size_t len);
}

#endif
