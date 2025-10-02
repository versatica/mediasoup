#include "RTC/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/Codecs/DependencyDescriptor.hpp"

class Listener : public ::RTC::Codecs::DependencyDescriptor::Listener
{
public:
	void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
	{
	}
};

void Fuzzer::RTC::Codecs::DependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::unique_ptr<::RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure> templateDependencyStructure;

	Listener listener;

	auto* descriptor = ::RTC::Codecs::DependencyDescriptor::Parse(
	  data, len, std::addressof(listener), templateDependencyStructure);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
