#include "RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/Consts.hpp"
#include <memory>

void FuzzerRtcRtpCodecsDependencyDescriptor::Fuzz(const uint8_t* data, size_t len)
{
	std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	const std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> descriptor{
		RTC::RTP::Codecs::DependencyDescriptor::Parse(data, len, templateDependencyStructure)
	};

	if (!descriptor)
	{
		return;
	}

	/*
	 * Rewrite it as the Consumers filtering layers do, which is what walks the
	 * bit offsets taken while parsing and must never write past what it counted.
	 */
	descriptor->GetMaxLength();

	uint8_t buffer[RTC::Consts::TwoBytesRtpExtensionMaxLength];

	for (uint8_t maxSpatialLayer{ 0 }; maxSpatialLayer < 4; ++maxSpatialLayer)
	{
		for (uint8_t maxTemporalLayer{ 0 }; maxTemporalLayer < 4; ++maxTemporalLayer)
		{
			descriptor->Write(
			  descriptor->GetDecodeTargetsBitmask(maxSpatialLayer, maxTemporalLayer), buffer);
		}
	}
}
