#include "RTC/RTP/Codecs/FuzzerH264.hpp"
#include "RTC/RTP/Codecs/H264.hpp"

void Fuzzer::RTC::RTP::Codecs::H264::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::RTP::Codecs::DependencyDescriptor* dependencyDescriptor{ nullptr };

	::RTC::RTP::Codecs::H264::PayloadDescriptor* descriptor =
	  ::RTC::RTP::Codecs::H264::Parse(data, len, dependencyDescriptor);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
