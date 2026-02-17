#include "RTC/RTP/Codecs/FuzzerOpus.hpp"
#include "RTC/RTP/Codecs/Opus.hpp"

void FuzzerRtcRtpCodecsOpus::Fuzz(const uint8_t* data, size_t len)
{
	const RTC::RTP::Codecs::Opus::PayloadDescriptor* descriptor =
	  RTC::RTP::Codecs::Opus::Parse(data, len);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
