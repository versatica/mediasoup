#include "RTC/RTP/Codecs/FuzzerH264.hpp"
#include "RTC/RTP/Codecs/H264.hpp"

void FuzzerRtcRtpCodecsH264::Fuzz(const uint8_t* data, size_t len)
{
	const RTC::RTP::Codecs::H264::PayloadDescriptor* descriptor =
	  RTC::RTP::Codecs::H264::Parse(data, len);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
