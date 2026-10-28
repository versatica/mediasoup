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
					  : Codecs::EncodingContext(params)
					{
					}
					~EncodingContext() override = default;

					/* Pure virtual methods inherited from Codecs::EncodingContext. */
				public:
					void SyncRequired() override
					{
						this->syncRequired = true;
					}

				public:
					bool syncRequired{ false };
				};
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
