#include "RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"

class Listener : public ::RTC::RTP::Codecs::DependencyDescriptor::Listener
{
public:
	void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
	{
	}
};

void Fuzzer::RTC::RTP::Codecs::DependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::unique_ptr<::RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	Listener listener;

	auto* descriptor = ::RTC::RTP::Codecs::DependencyDescriptor::Parse(
	  data, len, std::addressof(listener), templateDependencyStructure);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
