#include "RTC/Codecs/FuzzerAV1.hpp"
#include "RTC/Codecs/AV1.hpp"

void Fuzzer::RTC::Codecs::AV1::Fuzz(const uint8_t* data, size_t len)
{
	std::unique_ptr<::RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;

	auto* dependencyDescriptor =
	  ::RTC::Codecs::DependencyDescriptor::Parse(data, len, templateDependencyStructure);

	auto* descriptor = ::RTC::Codecs::AV1::Parse(dependencyDescriptor);

	delete dependencyDescriptor;

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
