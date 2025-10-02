#include "RTC/Codecs/FuzzerAV1.hpp"
#include "RTC/Codecs/AV1.hpp"

class Listener : public ::RTC::Codecs::DependencyDescriptor::Listener
{
public:
	void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
	{
	}
};

void Fuzzer::RTC::Codecs::AV1::Fuzz(const uint8_t* data, size_t len)
{
	Listener listener;
	std::unique_ptr<::RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;

	auto dependencyDescriptor =
	  std::unique_ptr<::RTC::Codecs::DependencyDescriptor>(::RTC::Codecs::DependencyDescriptor::Parse(
	    data, len, std::addressof(listener), templateDependencyStructure));

	auto* descriptor = ::RTC::Codecs::AV1::Parse(dependencyDescriptor);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
