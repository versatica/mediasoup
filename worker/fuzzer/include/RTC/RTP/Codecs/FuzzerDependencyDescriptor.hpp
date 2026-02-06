#ifndef MS_FUZZER_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP
#define MS_FUZZER_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"

namespace FuzzerRtcRtpCodecsDependencyDescriptor
{
	class DependencyDescriptorListener : public RTC::RTP::Codecs::DependencyDescriptor::Listener
	{
	public:
		void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override
		{
		}
	};

	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcRtpCodecsDependencyDescriptor

#endif
