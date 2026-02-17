#include "RTC/RTP/Codecs/FuzzerH264.hpp"
#include "RTC/RTP/Codecs/H264.hpp"

void FuzzerRtcRtpCodecsH264::Fuzz(const uint8_t* data, size_t len)
{
	RTC::RTP::Codecs::DependencyDescriptor* dependencyDescriptor{ nullptr };

	const RTC::RTP::Codecs::H264::PayloadDescriptor* descriptor =
	  RTC::RTP::Codecs::H264::Parse(data, len, dependencyDescriptor);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
