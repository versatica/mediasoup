#include "RTC/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/Codecs/DependencyDescriptor.hpp"

void Fuzzer::RTC::Codecs::DependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::unique_ptr<::RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;

	auto* descriptor =
	  ::RTC::Codecs::DependencyDescriptor::Parse(data, len, templateDependencyStructure);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
