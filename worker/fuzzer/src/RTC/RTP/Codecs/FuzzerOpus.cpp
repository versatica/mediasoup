#include "RTC/RTP/Codecs/FuzzerOpus.hpp"
#include "RTC/RTP/Codecs/Opus.hpp"

void Fuzzer::RTC::RTP::Codecs::Opus::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::RTP::Codecs::Opus::PayloadDescriptor* descriptor =
	  ::RTC::RTP::Codecs::Opus::Parse(data, len);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
