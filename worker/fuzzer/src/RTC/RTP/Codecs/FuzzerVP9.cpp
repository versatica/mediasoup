#include "RTC/RTP/Codecs/FuzzerVP9.hpp"
#include "RTC/RTP/Codecs/VP9.hpp"

void Fuzzer::RTC::RTP::Codecs::VP9::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::RTP::Codecs::VP9::PayloadDescriptor* descriptor = ::RTC::RTP::Codecs::VP9::Parse(data, len);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
