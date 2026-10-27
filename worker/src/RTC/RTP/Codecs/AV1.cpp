#define MS_CLASS "RTC::RTP::Codecs::AV1"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/AV1.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			AV1::PayloadDescriptor* AV1::Parse(
			  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor)
			{
				MS_TRACE();

				if (!dependencyDescriptor)
				{
					return nullptr;
				}

				auto* payloadDescriptor = new PayloadDescriptor(dependencyDescriptor);

				return payloadDescriptor;
			}

			void AV1::ProcessRtpPacket(
			  RTP::Packet* packet,
			  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>& templateDependencyStructure)
			{
				MS_TRACE();

				std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;

				// Read dependency descriptor.
				packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);

				PayloadDescriptor* payloadDescriptor = AV1::Parse(dependencyDescriptor);

				if (!payloadDescriptor)
				{
					return;
				}

				auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

				packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
			}

			/* Instance methods. */

			AV1::PayloadDescriptor::PayloadDescriptor(
			  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor)
			{
				MS_TRACE();

				// Read fields.
				this->startOfFrame  = dependencyDescriptor->startOfFrame;
				this->endOfFrame    = dependencyDescriptor->endOfFrame;
				this->frameNumber   = dependencyDescriptor->frameNumber;
				this->spatialLayer  = dependencyDescriptor->spatialLayer;
				this->temporalLayer = dependencyDescriptor->temporalLayer;
				this->isKeyFrame    = dependencyDescriptor->isKeyFrame;

				this->dependencyDescriptor = std::move(dependencyDescriptor);
			}

			void AV1::PayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<AV1::PayloadDescriptor>");
				this->dependencyDescriptor->Dump(indentation + 1);
				MS_DUMP_CLEAN(indentation, "</AV1::PayloadDescriptor>");
			}

			void AV1::PayloadDescriptor::Encode()
			{
				MS_TRACE();

				if (!this->encoder.has_value())
				{
					MS_WARN_DEV("there is no encoder present");

					return;
				}

				UpdateActiveDecodeTargets(
				  this->encoder->encodingData.maxSpatialLayer, this->encoder->encodingData.maxTemporalLayer);
			}

			void AV1::PayloadDescriptor::Restore() const
			{
				MS_TRACE();

				this->dependencyDescriptor->Restore();
			}

			// NOLINTNEXTLINE(readability-make-member-function-const)
			void AV1::PayloadDescriptor::UpdateActiveDecodeTargets(uint16_t spatialLayer, uint16_t temporalLayer)
			{
				MS_TRACE();

				this->dependencyDescriptor->UpdateActiveDecodeTargets(spatialLayer, temporalLayer);
			}

			void AV1::PayloadDescriptor::Encoder::Encode(PayloadDescriptor* payloadDescriptor) const
			{
				MS_TRACE();

				payloadDescriptor->UpdateActiveDecodeTargets(
				  this->encodingData.maxSpatialLayer, this->encodingData.maxTemporalLayer);
			}

			AV1::PayloadDescriptorHandler::PayloadDescriptorHandler(AV1::PayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool AV1::PayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* encodingContext, RTP::Packet* /*packet*/, bool& marker)
			{
				MS_TRACE();

				auto* context = static_cast<Codecs::AV1::EncodingContext*>(encodingContext);

				MS_ASSERT(context->GetTargetSpatialLayer() >= 0, "target spatial layer cannot be -1");
				MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

				auto packetSpatialLayer  = GetSpatialLayer();
				auto packetTemporalLayer = GetTemporalLayer();
				auto tmpSpatialLayer     = context->GetCurrentSpatialLayer();
				auto tmpTemporalLayer    = context->GetCurrentTemporalLayer();

				// If packet spatial or temporal layer is higher than maximum announced
				// one, drop the packet.
				if (packetSpatialLayer >= context->GetSpatialLayers() || packetTemporalLayer >= context->GetTemporalLayers())
				{
					MS_WARN_TAG(
					  rtp, "too high packet layers %" PRIu8 ":%" PRIu8, packetSpatialLayer, packetTemporalLayer);

					return false;
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
					if (packetSpatialLayer == context->GetTargetSpatialLayer() && this->payloadDescriptor->endOfFrame)
					{
						MS_DEBUG_DEV(
						  "downgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
						  ":%" PRIu8 ") without keyframe (full SVC)",
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
					return false;
				}

				// Upgrade current temporal layer if needed.
				if (context->GetTargetTemporalLayer() > context->GetCurrentTemporalLayer())
				{
					// NOTE: Only switch up in a frame that the Dependency Descriptor marks as
					// a switch point for the decode target we move into. Otherwise we'd
					// forward frames that depend on frames we already dropped, which the
					// endpoint cannot decode.
					//
					// NOTE: It must also be the lowest spatial layer of its picture. The
					// current temporal layer is common to all spatial layers, so switching up
					// in the middle of a picture would enable the new temporal layer for the
					// spatial layers of that same picture that were already dropped.
					if (
					  packetSpatialLayer == 0 && this->payloadDescriptor->IsSwitchPointForTemporalLayer(
					                               static_cast<uint8_t>(context->GetTargetTemporalLayer())))
					{
						MS_DEBUG_DEV(
						  "upgrading tmpTemporalLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
						  ":%" PRIu8 ")",
						  context->GetCurrentTemporalLayer(),
						  context->GetTargetTemporalLayer(),
						  packetSpatialLayer,
						  packetTemporalLayer);

						tmpTemporalLayer = context->GetTargetTemporalLayer();
					}
				}
				// Downgrade current temporal layer if needed.
				else if (context->GetTargetTemporalLayer() < context->GetCurrentTemporalLayer())
				{
					if (packetTemporalLayer == context->GetTargetTemporalLayer() && this->payloadDescriptor->endOfFrame)
					{
						MS_DEBUG_DEV(
						  "downgrading tmpTemporalLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
						  ":%" PRIu8 ")",
						  context->GetCurrentTemporalLayer(),
						  context->GetTargetTemporalLayer(),
						  packetSpatialLayer,
						  packetTemporalLayer);

						tmpTemporalLayer = packetTemporalLayer;
					}
				}

				// Filter temporal layers higher than current one.
				if (packetTemporalLayer > tmpTemporalLayer)
				{
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

				// Tell the endpoint which decode targets it must expect, since the ones
				// above the current layers are filtered out on purpose and it would
				// otherwise take their absence for loss.
				//
				// NOTE: The encoding data is stored so that retransmissions of this packet
				// carry the same bitmask.
				// clang-format off
				this->payloadDescriptor->CreateEncoder({
				  .maxSpatialLayer=static_cast<uint32_t>(context->GetCurrentSpatialLayer()),
				  .maxTemporalLayer=static_cast<uint32_t>(context->GetCurrentTemporalLayer())
				});
				// clang-format on
				this->payloadDescriptor->Encode();

				return true;
			}

			void AV1::PayloadDescriptorHandler::Encode(
			  RTP::Packet* /*packet*/, Codecs::PayloadDescriptor::Encoder* encoder)
			{
				MS_TRACE();

				auto* av1Encoder = static_cast<AV1::PayloadDescriptor::Encoder*>(encoder);

				av1Encoder->Encode(this->payloadDescriptor.get());
			}

			void AV1::PayloadDescriptorHandler::Restore(RTP::Packet* /*packet*/)
			{
				MS_TRACE();

				this->payloadDescriptor->Restore();
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
