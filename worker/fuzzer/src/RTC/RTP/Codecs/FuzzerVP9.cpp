#include "fuzzer/include/RTC/RTP/Codecs/FuzzerVP9.hpp"
#include "RTC/RTP/Codecs/VP9.hpp"

void FuzzerRtcRtpCodecsVP9::Fuzz(const uint8_t* data, size_t len)
{
	const std::unique_ptr<const RTC::RTP::Codecs::VP9::PayloadDescriptor> descriptor{
		RTC::RTP::Codecs::VP9::Parse(data, len)
	};
}
