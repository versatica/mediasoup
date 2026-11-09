#ifndef MS_RTC_RTP_CODECS_AV1_HPP
#define MS_RTC_RTP_CODECS_AV1_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			class AV1
			{
			private:
				/**
				 * Bits AV1 numbers its frames with. It does not number them itself, so
				 * they are the ones the Dependency Descriptor gives.
				 */
				static constexpr uint8_t FrameIdLength{ DependencyDescriptor::FrameIdLength };

			public:
				static void ProcessRtpPacket(
				  RTP::Packet* packet,
				  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
				    templateDependencyStructure);

			public:
				class EncodingContext : public Codecs::EncodingContext
				{
				public:
					explicit EncodingContext(Codecs::EncodingContext::Params& params)
					  : Codecs::EncodingContext(params, AV1::FrameIdLength)
					{
					}
					~EncodingContext() override = default;
				};
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
