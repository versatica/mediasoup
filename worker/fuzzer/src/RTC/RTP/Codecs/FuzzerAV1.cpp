#include "RTC/RTP/Codecs/FuzzerAV1.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"

class Listener : public ::RTC::RTP::Codecs::DependencyDescriptor::Listener
{
public:
	void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
	{
	}
};

void Fuzzer::RTC::RTP::Codecs::AV1::Fuzz(const uint8_t* data, size_t len)
{
	Listener listener;
	std::unique_ptr<::RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	auto dependencyDescriptor = std::unique_ptr<::RTC::RTP::Codecs::DependencyDescriptor>(
	  ::RTC::RTP::Codecs::DependencyDescriptor::Parse(
	    data, len, std::addressof(listener), templateDependencyStructure));

	auto* descriptor = ::RTC::RTP::Codecs::AV1::Parse(dependencyDescriptor);

	if (!descriptor)
	{
		return;
	}

	delete descriptor;
}
