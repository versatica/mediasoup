#include "RTC/RTP/Codecs/FuzzerVP8.hpp"
#include "RTC/RTP/Codecs/VP8.hpp"

void FuzzerRtcRtpCodecsVP8::Fuzz(const uint8_t* data, size_t len)
{
	const RTC::RTP::Codecs::VP8::PayloadDescriptor* descriptor =
	  RTC::RTP::Codecs::VP8::Parse(data, len);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
