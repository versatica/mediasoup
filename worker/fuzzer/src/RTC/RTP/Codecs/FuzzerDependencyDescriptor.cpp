#include "RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"

void FuzzerRtcRtpCodecsDependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	auto* descriptor =
	  RTC::RTP::Codecs::DependencyDescriptor::Parse(data, len, templateDependencyStructure);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
