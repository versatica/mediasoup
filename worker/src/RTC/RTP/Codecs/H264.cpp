#define MS_CLASS "RTC::RTP::Codecs::H264"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/H264.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			H264::PayloadDescriptor* H264::Parse(const uint8_t* data, size_t len)
			{
				MS_TRACE();

				std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

				payloadDescriptor->isKeyFrame = IsKeyFrame(data, len);

				return payloadDescriptor.release();
			}

			bool H264::IsKeyFrame(const uint8_t* data, size_t len)
			{
				MS_TRACE();

				if (len < 2)
				{
					MS_WARN_DEV("ignoring payload with length < 2");

					return false;
				}

				const uint8_t nal = *data & 0x1F;

				switch (nal)
				{
					// Single NAL unit packet.
					// IDR (instantaneous decoding picture).
					case 7:
					{
						return true;
					}

					// Aggreation packet.
					// STAP-A.
					case 24:
					{
						size_t offset{ 1 };

						len -= 1;

						// Iterate NAL units.
						while (len >= 3)
						{
							auto naluSize        = Utils::Byte::Get2Bytes(data, offset);
							const uint8_t subnal = *(data + offset + sizeof(naluSize)) & 0x1F;

							if (subnal == 7)
							{
								return true;
							}

							// Check if there is room for the indicated NAL unit size.
							if (len < (naluSize + sizeof(naluSize)))
							{
								break;
							}

							offset += naluSize + sizeof(naluSize);
							len -= naluSize + sizeof(naluSize);
						}

						break;
					}

					// Aggreation packet.
					// FU-A, FU-B.
					case 28:
					case 29:
					{
						const uint8_t subnal   = *(data + 1) & 0x1F;
						const uint8_t startBit = *(data + 1) & 0x80;

						if (subnal == 7 && startBit == 128)
						{
							return true;
						}

						break;
					}

					default:;
				}

				return false;
			}

			void H264::ProcessRtpPacket(
			  RTP::Packet* packet,
			  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>& templateDependencyStructure)
			{
				MS_TRACE();

				/*
				 * The Dependency Descriptor tells the layers of the frame and whether
				 * decoding can start at it, which is all that is needed to forward the
				 * packet, and it must be rewritten for the layers each Consumer is given.
				 * So it takes over whenever the packet carries one.
				 */
				auto* dependencyDescriptorPayloadDescriptor =
				  DependencyDescriptorPayloadDescriptor::Parse(packet, templateDependencyStructure);

				if (dependencyDescriptorPayloadDescriptor)
				{
					packet->SetPayloadDescriptorHandler(
					  new DependencyDescriptorPayloadDescriptorHandler(dependencyDescriptorPayloadDescriptor));

					return;
				}

				auto* data = packet->GetPayload();
				auto len   = packet->GetPayloadLength();

				PayloadDescriptor* payloadDescriptor = H264::Parse(data, len);

				if (!payloadDescriptor)
				{
					return;
				}

				auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

				packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
			}

			/* Instance methods. */

			void H264::PayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<H264::PayloadDescriptor>");
				MS_DUMP_CLEAN(indentation, "  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "</H264::PayloadDescriptor>");
			}

			H264::PayloadDescriptorHandler::PayloadDescriptorHandler(H264::PayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool H264::PayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* encodingContext, RTP::Packet* /*packet*/, bool& /*marker*/)
			{
				MS_TRACE();

				auto* context = static_cast<Codecs::H264::EncodingContext*>(encodingContext);

				MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

				// H264 always has a single spatial layer (0).
				context->SetCurrentSpatialLayer(0);

				/*
				 * The temporal layer is not parsed out of H264 packets, so they all are
				 * taken for the lowest one and none is ever filtered out.
				 *
				 * NOTE: The NAL unit header extensions that carry it in H264 SVC are not
				 * read. The Dependency Descriptor does tell it, and when the packet
				 * carries one its payload descriptor handler takes over this one.
				 */
				if (context->GetCurrentTemporalLayer() < 0)
				{
					context->SetCurrentTemporalLayer(0);
				}

				if (context->GetCurrentTemporalLayer() > context->GetTargetTemporalLayer())
				{
					context->SetCurrentTemporalLayer(context->GetTargetTemporalLayer());
				}

				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
