#define MS_CLASS "RTC::Codecs::H264"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/H264.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		H264::PayloadDescriptor* H264::Parse(
		  const uint8_t* data, size_t len, Codecs::DependencyDescriptor* dependencyDescriptor)
		{
			MS_TRACE();

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			if (dependencyDescriptor)
			{
				// Read fields.
				payloadDescriptor->startOfFrame  = dependencyDescriptor->startOfFrame;
				payloadDescriptor->endOfFrame    = dependencyDescriptor->endOfFrame;
				payloadDescriptor->spatialLayer  = dependencyDescriptor->spatialLayer;
				payloadDescriptor->temporalLayer = dependencyDescriptor->temporalLayer;

				payloadDescriptor->isKeyFrame = dependencyDescriptor->isKeyFrame;
			}
			else
			{
				payloadDescriptor->isKeyFrame = IsKeyFrame(data, len);
			}

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
			}

			return false;
		}

		void H264::ProcessRtpPacket(
		  RTC::RtpPacket* packet,
		  std::unique_ptr<RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure>&
		    templateDependencyStructure)
		{
			MS_TRACE();

			auto* data = packet->GetPayload();
			auto len   = packet->GetPayloadLength();
			std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;

			// Read dependency descriptor.
			packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);

			PayloadDescriptor* payloadDescriptor = H264::Parse(data, len, dependencyDescriptor.get());

			if (!payloadDescriptor)
			{
				return;
			}

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}

		/* Instance methods. */

		void H264::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<H264::PayloadDescriptor>");
			MS_DUMP("  startOfFrame:%" PRIu8 "|endOfFrame:%" PRIu8, this->startOfFrame, this->endOfFrame);
			MS_DUMP("  spatialLayer:%" PRIu8, this->spatialLayer);
			MS_DUMP("  temporalLayer:%" PRIu8, this->temporalLayer);
			MS_DUMP("  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
			MS_DUMP("</H264::PayloadDescriptor>");
		}

		H264::PayloadDescriptorHandler::PayloadDescriptorHandler(H264::PayloadDescriptor* payloadDescriptor)
		{
			MS_TRACE();

			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool H264::PayloadDescriptorHandler::Process(
		  RTC::Codecs::EncodingContext* encodingContext, RTC::RtpPacket* /*packet*/, bool& /*marker*/)
		{
			MS_TRACE();

			auto* context = static_cast<RTC::Codecs::H264::EncodingContext*>(encodingContext);

			MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

			if (this->payloadDescriptor->temporalLayer > context->GetTargetTemporalLayer())
			{
				return false;
			}

			// Update/fix current temporal layer.
			if (this->payloadDescriptor->temporalLayer > context->GetCurrentTemporalLayer())
			{
				context->SetCurrentTemporalLayer(this->payloadDescriptor->temporalLayer);
			}

			if (context->GetCurrentTemporalLayer() > context->GetTargetTemporalLayer())
			{
				context->SetCurrentTemporalLayer(context->GetTargetTemporalLayer());
			}

			return true;
		}
	} // namespace Codecs
} // namespace RTC
