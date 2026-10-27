#include "RTC/RTP/Codecs/FuzzerAV1.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"

void FuzzerRtcRtpCodecsAV1::Fuzz(const uint8_t* data, size_t len)
{
	std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	auto dependencyDescriptor = std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor>(
	  RTC::RTP::Codecs::DependencyDescriptor::Parse(data, len, templateDependencyStructure));

	auto* descriptor = RTC::RTP::Codecs::AV1::Parse(dependencyDescriptor);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
