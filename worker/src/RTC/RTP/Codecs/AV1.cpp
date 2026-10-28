#define MS_CLASS "RTC::RTP::Codecs::AV1"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/AV1.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			void AV1::ProcessRtpPacket(
			  RTP::Packet* packet,
			  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>& templateDependencyStructure)
			{
				MS_TRACE();

				/*
				 * NOTE: Nothing but the Dependency Descriptor tells the layers of an AV1
				 * packet, so without it the packet gets no payload descriptor handler at
				 * all and is forwarded as it comes.
				 */
				auto* payloadDescriptor =
				  DependencyDescriptorPayloadDescriptor::Parse(packet, templateDependencyStructure);

				if (!payloadDescriptor)
				{
					return;
				}

				packet->SetPayloadDescriptorHandler(
				  new DependencyDescriptorPayloadDescriptorHandler(payloadDescriptor));
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
