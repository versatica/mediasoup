#define MS_CLASS "RTC::RTP::Codecs::DependencyDescriptorPayloadDescriptor"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			DependencyDescriptorPayloadDescriptor* DependencyDescriptorPayloadDescriptor::Parse(
			  RTP::Packet* packet,
			  std::shared_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>& templateDependencyStructure)
			{
				MS_TRACE();

				std::unique_ptr<Codecs::DependencyDescriptor> dependencyDescriptor;

				packet->ReadDependencyDescriptor(dependencyDescriptor, templateDependencyStructure);

				// The packet carries no Dependency Descriptor, so its codec must make do
				// with its own payload descriptor.
				if (!dependencyDescriptor)
				{
					return nullptr;
				}

				return new DependencyDescriptorPayloadDescriptor(dependencyDescriptor);
			}

			/* Instance methods. */

			DependencyDescriptorPayloadDescriptor::DependencyDescriptorPayloadDescriptor(
			  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor)
			  : startOfFrame(dependencyDescriptor->startOfFrame),
			    endOfFrame(dependencyDescriptor->endOfFrame),
			    frameNumber(dependencyDescriptor->frameNumber),
			    spatialLayer(dependencyDescriptor->spatialLayer),
			    temporalLayer(dependencyDescriptor->temporalLayer),
			    dependencyDescriptor(std::move(dependencyDescriptor)),
			    isKeyFrame(this->dependencyDescriptor->isKeyFrame)
			{
				MS_TRACE();
			}

			void DependencyDescriptorPayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<DependencyDescriptorPayloadDescriptor>");
				this->dependencyDescriptor->Dump(indentation + 1);
				MS_DUMP_CLEAN(indentation, "</DependencyDescriptorPayloadDescriptor>");
			}

			void DependencyDescriptorPayloadDescriptor::Restore(RTP::Packet* packet) const
			{
				MS_TRACE();

				this->dependencyDescriptor->Restore(packet);
			}

			// NOLINTNEXTLINE(readability-make-member-function-const)
			void DependencyDescriptorPayloadDescriptor::UpdateActiveDecodeTargets(
			  uint16_t spatialLayer, uint16_t temporalLayer, RTP::Packet* packet)
			{
				MS_TRACE();

				this->dependencyDescriptor->UpdateActiveDecodeTargets(spatialLayer, temporalLayer, packet);
			}

			void DependencyDescriptorPayloadDescriptor::Encoder::Encode(
			  DependencyDescriptorPayloadDescriptor* payloadDescriptor, RTP::Packet* packet) const
			{
				MS_TRACE();

				payloadDescriptor->UpdateActiveDecodeTargets(
				  this->encodingData.maxSpatialLayer, this->encodingData.maxTemporalLayer, packet);
			}

			DependencyDescriptorPayloadDescriptorHandler::DependencyDescriptorPayloadDescriptorHandler(
			  DependencyDescriptorPayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool DependencyDescriptorPayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* encodingContext, RTP::Packet* packet, bool& marker)
			{
				MS_TRACE();

				auto* context = encodingContext;

				/*
				 * The spatial layer of a simulcast Consumer is chosen by picking the
				 * stream to forward, which the ProducerStreamManager does, so its target
				 * spatial layer is never set and there is nothing to filter here.
				 */
				const bool filterSpatialLayers = !context->IsSimulcast();

				MS_ASSERT(
				  !filterSpatialLayers || context->GetTargetSpatialLayer() >= 0,
				  "target spatial layer cannot be -1");
				MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

				const auto frameNumber = this->payloadDescriptor->frameNumber;
				/*
				 * A reordered packet must be judged by the layers its frame was given
				 * rather than by the current ones, which may have changed since. Letting
				 * it through on the new layers would forward a frame whose references
				 * were dropped.
				 *
				 * NOTE: Only where the spatial layers are filtered here, this is, not for
				 * a simulcast Consumer. There a stream switch brings frame numbers of a
				 * different encoder, so they do not order against the ones seen so far.
				 */
				const bool isOldPacket = filterSpatialLayers && context->IsOldFrameId(frameNumber);

				auto packetSpatialLayer  = GetSpatialLayer();
				auto packetTemporalLayer = GetTemporalLayer();
				auto tmpSpatialLayer     = isOldPacket ? context->GetSpatialLayerForFrameId(frameNumber)
				                                       : context->GetCurrentSpatialLayer();
				auto tmpTemporalLayer    = isOldPacket ? context->GetTemporalLayerForFrameId(frameNumber)
				                                       : context->GetCurrentTemporalLayer();

				if (!isOldPacket)
				{
					context->UpdateMaxFrameId(frameNumber);
				}

				// If packet temporal layer is higher than the maximum announced one, drop
				// the packet.
				if (packetTemporalLayer >= context->GetTemporalLayers())
				{
					MS_WARN_TAG(codec, "too high packet temporal layer %" PRIu8, packetTemporalLayer);

					return false;
				}

				// Same for the spatial one, whose maximum is the number of layers of the
				// stream rather than of the Consumer when it is not filtered here.
				if (filterSpatialLayers && packetSpatialLayer >= context->GetSpatialLayers())
				{
					MS_WARN_TAG(codec, "too high packet spatial layer %" PRIu8, packetSpatialLayer);

					return false;
				}

				// Move the current spatial layer towards the target one. A reordered packet
				// takes no part in it, since it says nothing about what the Consumer is
				// being given now.
				if (!isOldPacket && filterSpatialLayers)
				{
					// Upgrade current spatial layer if needed.
					if (context->GetTargetSpatialLayer() > context->GetCurrentSpatialLayer())
					{
						if (this->payloadDescriptor->isKeyFrame)
						{
							MS_DEBUG_DEV(
							  "upgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
							  ":%" PRIu8 ")",
							  context->GetCurrentSpatialLayer(),
							  context->GetTargetSpatialLayer(),
							  packetSpatialLayer,
							  packetTemporalLayer);

							// NOTE: The temporal layer is left alone. A key frame is a switch
							// point for every decode target, so the one of the new spatial
							// layer can be entered at whichever temporal layer is in use.
							tmpSpatialLayer = context->GetTargetSpatialLayer();
						}
					}
					// Downgrade current spatial layer if needed.
					else if (context->GetTargetSpatialLayer() < context->GetCurrentSpatialLayer())
					{
						/*
						 * The decode target we move into is the one of the target spatial
						 * layer at the temporal layer already in use, which is left alone so
						 * that a spatial change does not cost the frame rate the Consumer had
						 * climbed to.
						 *
						 * NOTE: A switch point is only good enough when the frames of that
						 * decode target were already reaching the endpoint, as in full SVC,
						 * where the lower spatial layers are required by the higher ones. A
						 * "switch" indication promises that no older frame of the decode
						 * target is needed, not that the frame stands on its own, and in
						 * K-SVC every frame of the target spatial layer has been dropped so
						 * far. There the switch waits for a key frame, as VP9 does.
						 */
						const bool atSwitchPoint =
						  std::cmp_equal(packetSpatialLayer, context->GetTargetSpatialLayer()) &&
						  this->payloadDescriptor->endOfFrame &&
						  this->payloadDescriptor->IsSwitchPointForDecodeTarget(
						    static_cast<uint8_t>(context->GetTargetSpatialLayer()),
						    static_cast<uint8_t>(tmpTemporalLayer));

						const bool targetAlreadySent = this->payloadDescriptor->IsDecodeTargetActive(
						  static_cast<uint8_t>(context->GetTargetSpatialLayer()),
						  static_cast<uint8_t>(tmpTemporalLayer),
						  static_cast<uint8_t>(context->GetCurrentSpatialLayer()),
						  static_cast<uint8_t>(tmpTemporalLayer));

						if (this->payloadDescriptor->isKeyFrame || (atSwitchPoint && targetAlreadySent))
						{
							MS_DEBUG_DEV(
							  "downgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
							  ":%" PRIu8 ") at a switch point, without keyframe",
							  context->GetCurrentSpatialLayer(),
							  context->GetTargetSpatialLayer(),
							  packetSpatialLayer,
							  packetTemporalLayer);

							tmpSpatialLayer = context->GetTargetSpatialLayer();
						}
					}
				}

				// Filter spatial layers higher than current one.
				if (filterSpatialLayers && std::cmp_greater(packetSpatialLayer, tmpSpatialLayer))
				{
					return false;
				}

				// Move the current temporal layer towards the target one, likewise.
				if (!isOldPacket)
				{
					// Upgrade current temporal layer if needed.
					if (context->GetTargetTemporalLayer() > context->GetCurrentTemporalLayer())
					{
						// NOTE: Only switch up in a frame that the Dependency Descriptor marks
						// as a switch point for the decode target we move into. Otherwise we'd
						// forward frames that depend on frames we already dropped, which the
						// endpoint cannot decode.
						//
						// NOTE: It must also be the lowest spatial layer of its picture. The
						// current temporal layer is common to all spatial layers, so switching
						// up in the middle of a picture would enable the new temporal layer for
						// the spatial layers of that same picture that were already dropped.
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
						if (
						  std::cmp_equal(packetTemporalLayer, context->GetTargetTemporalLayer()) &&
						  this->payloadDescriptor->endOfFrame)
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
				}

				// Filter temporal layers higher than current one.
				if (std::cmp_greater(packetTemporalLayer, tmpTemporalLayer))
				{
					return false;
				}

				// Set marker bit if needed.
				if (
				  (!filterSpatialLayers || std::cmp_equal(packetSpatialLayer, tmpSpatialLayer)) &&
				  this->payloadDescriptor->endOfFrame)
				{
					marker = true;
				}

				/*
				 * Update the current layers if needed, recording the frame number they
				 * change at so that packets reordered across the change are judged by
				 * the layers their own frame was given.
				 *
				 * NOTE: A reordered packet never moves them, since it says nothing about
				 * what the Consumer is being given now.
				 */
				if (!isOldPacket)
				{
					if (filterSpatialLayers && tmpSpatialLayer != context->GetCurrentSpatialLayer())
					{
						context->SetCurrentSpatialLayer(tmpSpatialLayer, frameNumber);
					}

					if (tmpTemporalLayer != context->GetCurrentTemporalLayer())
					{
						context->SetCurrentTemporalLayer(tmpTemporalLayer, frameNumber);
					}
				}

				/*
				 * Filter frames that the decode target being forwarded does not contain,
				 * which the layer comparisons above cannot tell since they only know that
				 * a frame is not above the wanted layers.
				 *
				 * NOTE: This comes after the current layers are updated because the layer
				 * switching above watches every frame of the stream, including those that
				 * are not forwarded. In K-SVC the switch points of a decode target may
				 * well be frames that the endpoint is never sent.
				 */
				const bool dtiFilterApplies =
				  filterSpatialLayers && tmpSpatialLayer >= 0 && tmpTemporalLayer >= 0;
				const bool presentInDecodeTarget =
				  !dtiFilterApplies ||
				  this->payloadDescriptor->IsPresentInDecodeTarget(
				    static_cast<uint8_t>(tmpSpatialLayer), static_cast<uint8_t>(tmpTemporalLayer));

				if (!presentInDecodeTarget)
				{
					return false;
				}

				// Tell the endpoint which decode targets it must expect, since the ones
				// above the current layers are filtered out on purpose and it would
				// otherwise take their absence for loss.
				//
				// NOTE: The encoding data is stored so that retransmissions of this packet
				// carry the same bitmask.
				/*
				 * NOTE: When the spatial layers are not filtered here they all are
				 * forwarded, so the endpoint must be told that every decode target of
				 * the stream is active rather than only those of one spatial layer.
				 */
				const auto maxSpatialLayer = filterSpatialLayers
				                               ? static_cast<uint32_t>(context->GetCurrentSpatialLayer())
				                               : this->payloadDescriptor->GetSpatialLayers();

				const auto maxTemporalLayer = static_cast<uint32_t>(context->GetCurrentTemporalLayer());

				this->payloadDescriptor->UpdateActiveDecodeTargets(maxSpatialLayer, maxTemporalLayer, packet);

				// Leave the Consumer how this packet was rewritten for it.
				// clang-format off
				auto encoder = std::make_unique<DependencyDescriptorPayloadDescriptor::Encoder>(
				  DependencyDescriptorPayloadDescriptor::EncodingData{
				    .maxSpatialLayer=maxSpatialLayer,
				    .maxTemporalLayer=maxTemporalLayer
				  });
				// clang-format on

				encoder->preferDependencyDescriptor = context->PrefersDependencyDescriptor();

				context->encoder = std::move(encoder);

				return true;
			}

			void DependencyDescriptorPayloadDescriptorHandler::Encode(
			  RTP::Packet* packet, Codecs::PayloadDescriptor::Encoder* encoder)
			{
				MS_TRACE();

				auto* dependencyDescriptorEncoder =
				  static_cast<DependencyDescriptorPayloadDescriptor::Encoder*>(encoder);

				dependencyDescriptorEncoder->Encode(this->payloadDescriptor.get(), packet);
			}

			void DependencyDescriptorPayloadDescriptorHandler::Restore(RTP::Packet* packet)
			{
				MS_TRACE();

				this->payloadDescriptor->Restore(packet);
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
