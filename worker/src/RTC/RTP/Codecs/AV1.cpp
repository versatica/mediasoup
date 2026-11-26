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

			AV1::PayloadDescriptor* AV1::Parse(const uint8_t* data, size_t len)
			{
				MS_TRACE();

				if (len < 1)
				{
					MS_WARN_DEV("ignoring payload with no aggregation header");

					return nullptr;
				}

				auto* const payloadDescriptor = new PayloadDescriptor();

				const uint8_t byte = data[0];

				payloadDescriptor->z = (byte & 0x80) != 0;
				payloadDescriptor->y = (byte & 0x40) != 0;
				payloadDescriptor->w = static_cast<uint8_t>((uint32_t{ byte } & 0x30) >> 4);
				payloadDescriptor->n = (byte & 0x08) != 0;

				// A coded video sequence starts with a key frame, and it cannot start in
				// the middle of an OBU, so a packet that continues one is not its first.
				payloadDescriptor->isKeyFrame = payloadDescriptor->n && !payloadDescriptor->z;

				return payloadDescriptor;
			}

			bool AV1::ProcessRtpPacket(
			  RTP::Packet* packet, Codecs::DependencyDescriptor::Context& dependencyDescriptorContext)
			{
				MS_TRACE();

				if (packet->HasDependencyDescriptor())
				{
					auto* const dependencyDescriptorPayloadDescriptor =
					  DependencyDescriptorPayloadDescriptor::Parse(packet, dependencyDescriptorContext);

					// The descriptor could not be parsed.
					if (!dependencyDescriptorPayloadDescriptor)
					{
						return false;
					}

					// NOTE: Set as the handler of the codec, so that it also serves a
					// Consumer that did not negotiate the extension, which the payload
					// descriptor of AV1 cannot do since it tells nothing about the layers.
					// That Consumer is sent no Dependency Descriptor either, and an endpoint
					// without one takes every frame it gets for spatial layer 0, so sending
					// it several spatial layers would mix them up. Filtering them here is
					// what keeps its video decodable. See
					// https://issues.webrtc.org/issues/560293075.
					packet->SetPayloadDescriptorHandler(
					  std::make_unique<DependencyDescriptorPayloadDescriptorHandler>(
					    dependencyDescriptorPayloadDescriptor));
				}
				else
				{
					// Nothing but the Dependency Descriptor tells the layers of an AV1
					// packet, so without it the packet is forwarded as it comes. The
					// aggregation header still tells whether the packet starts a key frame,
					// which is what a Consumer waits for before forwarding anything.
					auto* const payloadDescriptor =
					  AV1::Parse(packet->GetPayload(), packet->GetPayloadLength());

					if (payloadDescriptor)
					{
						packet->SetPayloadDescriptorHandler(
						  std::make_unique<PayloadDescriptorHandler>(payloadDescriptor));
					}
				}

				return true;
			}

			/* Instance methods. */

			void AV1::PayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<AV1::PayloadDescriptor>");
				MS_DUMP_CLEAN(
				  indentation,
				  "  z:%" PRIu8 "|y:%" PRIu8 "|w:%" PRIu8 "|n:%" PRIu8,
				  this->z,
				  this->y,
				  this->w,
				  this->n);
				MS_DUMP_CLEAN(indentation, "  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "</AV1::PayloadDescriptor>");
			}

			AV1::PayloadDescriptorHandler::PayloadDescriptorHandler(AV1::PayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool AV1::PayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* /*encodingContext*/, RTP::Packet* /*packet*/, bool& /*marker*/)
			{
				MS_TRACE();

				// There is no Dependency Descriptor, so the layers of the packet are
				// unknown and nothing can be filtered out.
				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
