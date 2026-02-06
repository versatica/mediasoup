#include "RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"

void FuzzerRtcRtpCodecsDependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	DependencyDescriptorListener listener;

	auto* descriptor = RTC::RTP::Codecs::DependencyDescriptor::Parse(
	  data, len, std::addressof(listener), templateDependencyStructure);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
