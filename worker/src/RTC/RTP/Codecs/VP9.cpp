#define MS_CLASS "RTC::RTP::Codecs::VP9"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/VP9.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			VP9::PayloadDescriptor* VP9::Parse(const uint8_t* data, size_t len)
			{
				MS_TRACE();

				if (len < 1)
				{
					MS_WARN_DEV("ignoring empty payload");

					return nullptr;
				}

				auto payloadDescriptor = std::make_unique<PayloadDescriptor>();

				size_t offset{ 0 };
				uint8_t byte = data[offset];

				payloadDescriptor->i = (byte >> 7) & 0x01;
				payloadDescriptor->p = (byte >> 6) & 0x01;
				payloadDescriptor->l = (byte >> 5) & 0x01;
				payloadDescriptor->f = (byte >> 4) & 0x01;
				payloadDescriptor->b = (byte >> 3) & 0x01;
				payloadDescriptor->e = (byte >> 2) & 0x01;
				payloadDescriptor->v = (byte >> 1) & 0x01;

				if (payloadDescriptor->i)
				{
					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (1)");

						return nullptr;
					}

					byte = data[offset];

					if (byte >> 7 & 0x01)
					{
						if (len < ++offset + 1)
						{
							MS_WARN_DEV("ignoring invalid payload (2)");

							return nullptr;
						}

						payloadDescriptor->pictureId = (byte & 0x7F) << 8;
						payloadDescriptor->pictureId += data[offset];
						payloadDescriptor->hasTwoBytesPictureId = true;
					}
					else
					{
						payloadDescriptor->pictureId = byte & 0x7F;
					}

					payloadDescriptor->hasPictureId = true;
				}

				if (payloadDescriptor->l)
				{
					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (3)");

						return nullptr;
					}

					byte = data[offset];

					payloadDescriptor->interLayerDependency = byte & 0x01;
					payloadDescriptor->switchingUpPoint     = byte >> 4 & 0x01;
					payloadDescriptor->slIndex              = byte >> 1 & 0x07;
					payloadDescriptor->tlIndex              = byte >> 5 & 0x07;
					payloadDescriptor->hasSlIndex           = true;
					payloadDescriptor->hasTlIndex           = true;

					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (4)");

						return nullptr;
					}

					// Read TL0PICIDX if flexible mode is unset.
					if (!payloadDescriptor->f)
					{
						payloadDescriptor->tl0PictureIndex    = data[offset];
						payloadDescriptor->hasTl0PictureIndex = true;
					}
				}

				if (!payloadDescriptor->p && payloadDescriptor->b && payloadDescriptor->slIndex == 0)
				{
					payloadDescriptor->isKeyFrame = true;
				}

				return payloadDescriptor.release();
			}

			bool VP9::ProcessRtpPacket(
			  RTP::Packet* packet,
			  Codecs::DependencyDescriptor::Context& dependencyDescriptorContext,
			  Codecs::PictureIdUnwrapper& pictureIdUnwrapper)
			{
				MS_TRACE();

				// NOTE: The payload descriptor of the codec is also parsed, for the
				// Consumers that did not negotiate the Dependency Descriptor RTP header
				// extension.
				if (packet->HasDependencyDescriptor())
				{
					std::unique_ptr<DependencyDescriptorPayloadDescriptor> dependencyDescriptorPayloadDescriptor{
						DependencyDescriptorPayloadDescriptor::Parse(packet, dependencyDescriptorContext)
					};

					// The descriptor could not be parsed.
					if (!dependencyDescriptorPayloadDescriptor)
					{
						return false;
					}

					packet->SetDependencyDescriptorPayloadDescriptorHandler(
					  std::make_unique<DependencyDescriptorPayloadDescriptorHandler>(
					    std::move(dependencyDescriptorPayloadDescriptor)));
				}

				auto* data = packet->GetPayload();
				auto len   = packet->GetPayloadLength();

				PayloadDescriptor* payloadDescriptor = VP9::Parse(data, len);

				if (!payloadDescriptor)
				{
					return true;
				}

				// Extend the pictureId into the 15 bits, that is, carrying the cycles a 7
				// bits one has already wrapped.
				//
				// NOTE: Only the parsed value is extended. Nothing widens or rewrites the
				// field, so the width the sender used is kept and the worker at the other
				// end of a pipe extends the pictureId on its own.
				if (payloadDescriptor->hasPictureId)
				{
					payloadDescriptor->pictureId = pictureIdUnwrapper.Unwrap(
					  payloadDescriptor->pictureId, payloadDescriptor->hasTwoBytesPictureId);
				}

				if (payloadDescriptor->isKeyFrame)
				{
					MS_DEBUG_DEV(
					  "key frame [spatialLayer:%" PRIu8 ", temporalLayer:%" PRIu8 "]",
					  packet->GetSpatialLayer(),
					  packet->GetTemporalLayer());
				}

				packet->SetPayloadDescriptorHandler(
				  std::make_unique<PayloadDescriptorHandler>(payloadDescriptor));

				return true;
			}

			/* Instance methods. */

			void VP9::PayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<VP9::PayloadDescriptor>");
				MS_DUMP_CLEAN(
				  indentation,
				  "  i:%" PRIu8 "|p:%" PRIu8 "|l:%" PRIu8 "|f:%" PRIu8 "|b:%" PRIu8 "|e:%" PRIu8 "|v:%" PRIu8,
				  this->i,
				  this->p,
				  this->l,
				  this->f,
				  this->b,
				  this->e,
				  this->v);
				MS_DUMP_CLEAN(indentation, "  pictureId: %" PRIu16, this->pictureId);
				MS_DUMP_CLEAN(indentation, "  slIndex: %" PRIu8, this->slIndex);
				MS_DUMP_CLEAN(indentation, "  tlIndex: %" PRIu8, this->tlIndex);
				MS_DUMP_CLEAN(indentation, "  tl0PictureIndex: %" PRIu8, this->tl0PictureIndex);
				MS_DUMP_CLEAN(indentation, "  interLayerDependency: %" PRIu8, this->interLayerDependency);
				MS_DUMP_CLEAN(indentation, "  switchingUpPoint: %" PRIu8, this->switchingUpPoint);
				MS_DUMP_CLEAN(indentation, "  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  hasPictureId: %s", this->hasPictureId ? "true" : "false");
				MS_DUMP_CLEAN(
				  indentation, "  hasTwoBytesPictureId: %s", this->hasTwoBytesPictureId ? "true" : "false");
				MS_DUMP_CLEAN(
				  indentation, "  hasTl0PictureIndex: %s", this->hasTl0PictureIndex ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  hasSlIndex: %s", this->hasSlIndex ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  hasTlIndex: %s", this->hasTlIndex ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "</VP9::PayloadDescriptor>");
			}

			VP9::PayloadDescriptorHandler::PayloadDescriptorHandler(VP9::PayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool VP9::PayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* encodingContext, RTP::Packet* /*packet*/, bool& marker)
			{
				MS_TRACE();

				auto* context = static_cast<Codecs::VP9::EncodingContext*>(encodingContext);

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
					  codec, "too high packet layers %" PRIu8 ":%" PRIu8, packetSpatialLayer, packetTemporalLayer);

					return false;
				}

				// NOTE: `ProcessRtpPacket()` already extended the pictureId into the 15
				// bits, so it is used as it comes from here on.

				// Check whether pictureId sync is required.
				if (context->syncRequired && this->payloadDescriptor->hasPictureId)
				{
					context->pictureIdManager.Sync(this->payloadDescriptor->pictureId - 1);

					context->syncRequired = false;
				}

				const bool isOldPacket =
				  (this->payloadDescriptor->hasPictureId &&
					 RTC::SeqManager<uint16_t, VP9::FrameIdLength>::IsSeqLowerThan(
				     this->payloadDescriptor->pictureId, context->pictureIdManager.GetMaxInput()));

				if (!isOldPacket)
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

							tmpSpatialLayer  = context->GetTargetSpatialLayer();
							tmpTemporalLayer = 0; // Just in case.
						}
					}
					// Downgrade current spatial layer if needed.
					else if (context->GetTargetSpatialLayer() < context->GetCurrentSpatialLayer())
					{
						// In K-SVC we must wait for a keyframe.
						if (context->IsKSvc())
						{
							if (this->payloadDescriptor->isKeyFrame)
							{
								MS_DEBUG_DEV(
								  "downgrading tmpSpatialLayer from %" PRIu16 " to %" PRIu16 " (packet:%" PRIu8
								  ":%" PRIu8 ") after keyframe (K-SVC)",
								  context->GetCurrentSpatialLayer(),
								  context->GetTargetSpatialLayer(),
								  packetSpatialLayer,
								  packetTemporalLayer);

								tmpSpatialLayer  = context->GetTargetSpatialLayer();
								tmpTemporalLayer = 0; // Just in case.
							}
						}
						// In full SVC we do not need a keyframe.
						else
						{
							if (
							  std::cmp_equal(packetSpatialLayer, context->GetTargetSpatialLayer()) &&
							  this->payloadDescriptor->e)
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
					}
				}

				// Filter spatial layers that are either
				// * higher than current one
				// * different than the current one when KSVC is enabled and this is not a keyframe
				// (interframe p bit = 1)
				const uint16_t spatialLayerForFrameId =
				  isOldPacket ? context->GetSpatialLayerForFrameId(this->payloadDescriptor->pictureId)
					            : tmpSpatialLayer;

				if (
				  packetSpatialLayer > spatialLayerForFrameId ||
				  (context->IsKSvc() && this->payloadDescriptor->p &&
					 packetSpatialLayer != spatialLayerForFrameId))
				{
					return false;
				}

				// Check and handle temporal layer (unless old packet).
				if (!isOldPacket)
				{
					// Upgrade current temporal layer if needed.
					if (context->GetTargetTemporalLayer() > context->GetCurrentTemporalLayer())
					{
						if (
						  packetTemporalLayer >= context->GetCurrentTemporalLayer() + 1 &&
						  (context->GetCurrentTemporalLayer() == -1 || this->payloadDescriptor->switchingUpPoint) &&
						  this->payloadDescriptor->b)
						{
							MS_DEBUG_DEV(
							  "upgrading tmpTemporalLayer from %" PRIu16 " to %" PRIu8 " (packet:%" PRIu8
							  ":%" PRIu8 ")",
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
						if (
						  std::cmp_equal(packetTemporalLayer, context->GetTargetTemporalLayer()) &&
						  this->payloadDescriptor->e)
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
				}

				// Filter temporal layers higher than current one.
				const uint16_t temporalLayerForFrameId =
				  isOldPacket ? context->GetTemporalLayerForFrameId(this->payloadDescriptor->pictureId)
					            : tmpTemporalLayer;

				if (packetTemporalLayer > temporalLayerForFrameId)
				{
					return false;
				}

				// Set marker bit if needed.
				if (std::cmp_equal(packetSpatialLayer, tmpSpatialLayer) && this->payloadDescriptor->e)
				{
					marker = true;
				}

				// Update the pictureId manager.
				if (this->payloadDescriptor->hasPictureId)
				{
					uint16_t pictureId;

					// Nothing drops on this manager, so `Input()` always generates an output
					// which is not used since it does not rewrite the pictureId.
					static_cast<void>(
					  context->pictureIdManager.Input(this->payloadDescriptor->pictureId, pictureId));
				}

				// Update current spatial layer if needed.
				if (tmpSpatialLayer != context->GetCurrentSpatialLayer())
				{
					context->SetCurrentSpatialLayer(tmpSpatialLayer, this->payloadDescriptor->pictureId);
				}

				// Update current temporal layer if needed.
				if (tmpTemporalLayer != context->GetCurrentTemporalLayer())
				{
					context->SetCurrentTemporalLayer(tmpTemporalLayer, this->payloadDescriptor->pictureId);
				}

				return true;
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
