#define MS_CLASS "RTC::Codecs::AV1"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Codecs/AV1.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		AV1::PayloadDescriptor* AV1::Parse(std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor)
		{
			MS_TRACE();

			if (!dependencyDescriptor)
			{
				return nullptr;
			}

			auto* payloadDescriptor = new PayloadDescriptor(std::move(dependencyDescriptor));

			return payloadDescriptor;
		}

		void AV1::ProcessRtpPacket(
		  RTC::RtpPacket* packet,
		  std::unique_ptr<RTC::Codecs::DependencyDescriptor::TemplateDependencyStructure>&
		    templateDependencyStructure)
		{
			MS_TRACE();

			std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;

			// Read dependency descriptor.
			packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);

			PayloadDescriptor* payloadDescriptor = AV1::Parse(std::move(dependencyDescriptor));

			if (!payloadDescriptor)
			{
				return;
			}

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}

		/* Instance methods. */

		AV1::PayloadDescriptor::PayloadDescriptor(
		  std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor)
		{
			MS_TRACE();

			// Read fields.
			this->startOfFrame  = dependencyDescriptor->startOfFrame;
			this->endOfFrame    = dependencyDescriptor->endOfFrame;
			this->spatialLayer  = dependencyDescriptor->spatialLayer;
			this->temporalLayer = dependencyDescriptor->temporalLayer;
			this->isKeyFrame    = dependencyDescriptor->isKeyFrame;

			this->dependencyDescriptor = std::move(dependencyDescriptor);
		}

		void AV1::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<AV1::PayloadDescriptor>");
			MS_DUMP("  startOfFrame:%" PRIu8 "|endOfFrame:%" PRIu8, this->startOfFrame, this->endOfFrame);
			MS_DUMP("  spatialLayer: %" PRIu8, this->spatialLayer);
			MS_DUMP("  temporalLayer: %" PRIu8, this->temporalLayer);
			MS_DUMP("  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
			MS_DUMP("</AV1::PayloadDescriptor>");
		}

		void AV1::PayloadDescriptor::Encode(uint8_t* data, uint16_t frameNumber) const
		{
			MS_TRACE();

			this->dependencyDescriptor->SetFrameNumber(frameNumber);
		}

		void AV1::PayloadDescriptor::Encode(uint8_t* data) const
		{
			MS_TRACE();

			this->encoder->Encode(data, this);
		}

		void AV1::PayloadDescriptor::Restore(uint8_t* data) const
		{
			MS_TRACE();

			Encode(data, this->dependencyDescriptor->frameNumber);
		}

		void AV1::PayloadDescriptor::Encoder::Encode(
		  uint8_t* data, const PayloadDescriptor* payloadDescriptor) const
		{
			payloadDescriptor->Encode(data, this->encodingData.frameNumber);
		}

		AV1::PayloadDescriptorHandler::PayloadDescriptorHandler(AV1::PayloadDescriptor* payloadDescriptor)
		{
			MS_TRACE();

			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool AV1::PayloadDescriptorHandler::Process(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* /*data*/, bool& marker)
		{
			MS_TRACE();

			auto* context = static_cast<RTC::Codecs::AV1::EncodingContext*>(encodingContext);

			MS_ASSERT(context->GetTargetSpatialLayer() >= 0, "target spatial layer cannot be -1");
			MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

			auto packetSpatialLayer  = GetSpatialLayer();
			auto packetTemporalLayer = GetTemporalLayer();
			auto tmpSpatialLayer     = context->GetCurrentSpatialLayer();
			auto tmpTemporalLayer    = context->GetCurrentTemporalLayer();

			// If packet spatial or temporal layer is higher than maximum announced
			// one, drop the packet.
			// clang-format off
			if (
				packetSpatialLayer >= context->GetSpatialLayers() ||
				packetTemporalLayer >= context->GetTemporalLayers()
			)
			// clang-format on
			{
				MS_WARN_TAG(
				  rtp, "too high packet layers %" PRIu8 ":%" PRIu8, packetSpatialLayer, packetTemporalLayer);

				return false;
			}

			// Check whether frameNumber sync is required.
			if (context->syncRequired)
			{
				context->frameNumberManager.Sync(
				  this->payloadDescriptor->dependencyDescriptor->frameNumber - 1);

				context->syncRequired = false;
			}

			// Upgrade current spatial layer if needed.
			if (context->GetTargetSpatialLayer() > context->GetCurrentSpatialLayer())
			{
				if (this->payloadDescriptor->isKeyFrame)
				{
					MS_DEBUG_DEV(
					  "upgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8 ":%" PRIu8
					  ")",
					  context->GetCurrentSpatialLayer(),
					  context->GetTargetSpatialLayer(),
					  packetSpatialLayer,
					  packetTemporalLayer);

					tmpSpatialLayer  = context->GetTargetSpatialLayer();
					tmpTemporalLayer = 0; // Just in case.
				}
			}
			// Downgrade current spatial layer if needed.
			else if (context->GetTargetSpatialLayer() < context->GetCurrentSpatialLayer())
			{
				// clang-format off
					if (
						packetSpatialLayer == context->GetTargetSpatialLayer() &&
						this->payloadDescriptor->endOfFrame
					)
				// clang-format on
				{
					MS_DEBUG_DEV(
					  "downgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8 ":%" PRIu8
					  ") without keyframe (full SVC)",
					  context->GetCurrentSpatialLayer(),
					  context->GetTargetSpatialLayer(),
					  packetSpatialLayer,
					  packetTemporalLayer);

					tmpSpatialLayer  = context->GetTargetSpatialLayer();
					tmpTemporalLayer = 0; // Just in case.
				}
			}

			// Filter spatial layers higher than current one.
			if (packetSpatialLayer > tmpSpatialLayer)
			{
				context->frameNumberManager.Drop(
				  this->payloadDescriptor->dependencyDescriptor->frameNumber - 1);

				return false;
			}

			// Upgrade current temporal layer if needed.
			if (context->GetTargetTemporalLayer() > context->GetCurrentTemporalLayer())
			{
				// clang-format off
					if (
						packetTemporalLayer >= context->GetCurrentTemporalLayer() + 1)
				// clang-format on
				{
					MS_DEBUG_DEV(
					  "upgrading tmpTemporalLayer from %" PRIu16 " to %" PRIu8 " (packet:%" PRIu8 ":%" PRIu8
					  ")",
					  context->GetCurrentTemporalLayer(),
					  packetTemporalLayer,
					  packetSpatialLayer,
					  packetTemporalLayer);

					tmpTemporalLayer = packetTemporalLayer;
				}
			}
			// Downgrade current temporal layer if needed.
			else if (context->GetTargetTemporalLayer() < context->GetCurrentTemporalLayer())
			{
				// clang-format off
					if (
						packetTemporalLayer == context->GetTargetTemporalLayer() &&
						this->payloadDescriptor->endOfFrame
					)
				// clang-format on
				{
					MS_DEBUG_DEV(
					  "downgrading tmpTemporalLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
					  ":%" PRIu8 ")",
					  context->GetCurrentTemporalLayer(),
					  context->GetTargetTemporalLayer(),
					  packetSpatialLayer,
					  packetTemporalLayer);

					tmpTemporalLayer = context->GetTargetTemporalLayer();
				}
			}

			// Filter temporal layers higher than current one.
			if (packetTemporalLayer > tmpTemporalLayer)
			{
				context->frameNumberManager.Drop(
				  this->payloadDescriptor->dependencyDescriptor->frameNumber - 1);

				return false;
			}

			// Set marker bit if needed.
			if (packetSpatialLayer == tmpSpatialLayer && this->payloadDescriptor->endOfFrame)
			{
				marker = true;
			}

			// Update current spatial layer if needed.
			if (tmpSpatialLayer != context->GetCurrentSpatialLayer())
			{
				context->SetCurrentSpatialLayer(tmpSpatialLayer);
			}

			// Update current temporal layer if needed.
			if (tmpTemporalLayer != context->GetCurrentTemporalLayer())
			{
				context->SetCurrentTemporalLayer(tmpTemporalLayer);
			}

			return true;
		}

		void AV1::PayloadDescriptorHandler::Encode(uint8_t* data, Codecs::PayloadDescriptor::Encoder* encoder)
		{
			MS_TRACE();

			auto* av1Encoder = static_cast<AV1::PayloadDescriptor::Encoder*>(encoder);

			av1Encoder->Encode(data, this->payloadDescriptor.get());
		}

		void AV1::PayloadDescriptorHandler::Restore(uint8_t* /*data*/)
		{
			MS_TRACE();
		}
	} // namespace Codecs
} // namespace RTC
