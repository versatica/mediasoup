#define MS_CLASS "RTC::Codecs::VP9"
// #define MS_LOG_DEV

#include "RTC/Codecs/VP9.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		VP9::PayloadDescriptor* VP9::Parse(
		  const uint8_t* data,
		  size_t len,
		  RTC::RtpPacket::FrameMarking* /*frameMarking*/,
		  uint8_t /*frameMarkingLen*/)
		{
			MS_TRACE();

			if (len < 1)
				return nullptr;

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

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
					return nullptr;

				byte = data[offset];

				if (byte >> 7 & 0x01)
				{
					if (len < ++offset + 1)
						return nullptr;

					payloadDescriptor->pictureId = (byte & 0x7f) << 8;
					payloadDescriptor->pictureId += data[offset];
					payloadDescriptor->hasTwoBytesPictureId = true;
				}
				else
				{
					payloadDescriptor->pictureId           = byte & 0x7f;
					payloadDescriptor->hasOneBytePictureId = true;
				}

				payloadDescriptor->hasPictureId = true;
			}

			if (payloadDescriptor->l)
			{
				if (len < ++offset + 1)
					return nullptr;

				byte = data[offset];

				payloadDescriptor->interLayerDependency = byte & 0x01;
				payloadDescriptor->switchingUpPoint     = byte >> 4 & 0x01;
				payloadDescriptor->slIndex              = byte >> 1 & 0x07;
				payloadDescriptor->tlIndex              = byte >> 5 & 0x07;
				payloadDescriptor->hasSlIndex           = true;
				payloadDescriptor->hasTlIndex           = true;

				if (len < ++offset + 1)
					return nullptr;

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

		void VP9::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto* data = packet->GetPayload();
			auto len   = packet->GetPayloadLength();
			RtpPacket::FrameMarking* frameMarking{ nullptr };
			uint8_t frameMarkingLen{ 0 };

			// Read frame-marking.
			packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

			// TODO: TMP
			if (frameMarking)
			{
				MS_ERROR("frameMarking in VP9!!!");
			}

			PayloadDescriptor* payloadDescriptor = VP9::Parse(data, len, frameMarking, frameMarkingLen);

			if (!payloadDescriptor)
				return;

			// TODO: TMP
			// if (payloadDescriptor->l)
			// payloadDescriptor->Dump();

			// TODO: TMP
			if (payloadDescriptor->isKeyFrame)
				MS_ERROR("key frame!!");

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}

		/* Instance methods. */

		void VP9::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<PayloadDescriptor>");
			MS_DEBUG_DEV(
			  "  i:%" PRIu8 "|p:%" PRIu8 "|l:%" PRIu8 "|f:%" PRIu8 "|b:%" PRIu8 "|e:%" PRIu8 "|v:%" PRIu8,
			  this->i,
			  this->p,
			  this->l,
			  this->f,
			  this->b,
			  this->e,
			  this->v);
			MS_DEBUG_DEV("  pictureId            : %" PRIu16, this->pictureId);
			MS_DEBUG_DEV("  slIndex              : %" PRIu8, this->slIndex);
			MS_DEBUG_DEV("  tlIndex              : %" PRIu8, this->tlIndex);
			MS_DEBUG_DEV("  tl0PictureIndex      : %" PRIu8, this->tl0PictureIndex);
			MS_DEBUG_DEV("  interLayerDependency : %" PRIu8, this->interLayerDependency);
			MS_DEBUG_DEV("  switchingUpPoint     : %" PRIu8, this->switchingUpPoint);
			MS_DEBUG_DEV("  isKeyFrame           : %s", this->isKeyFrame ? "true" : "false");
			MS_DEBUG_DEV("  hasPictureId         : %s", this->hasPictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasOneBytePictureId  : %s", this->hasOneBytePictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasTwoBytesPictureId : %s", this->hasTwoBytesPictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasTl0PictureIndex   : %s", this->hasTl0PictureIndex ? "true" : "false");
			MS_DEBUG_DEV("  hasSlIndex           : %s", this->hasSlIndex ? "true" : "false");
			MS_DEBUG_DEV("  hasTlIndex           : %s", this->hasTlIndex ? "true" : "false");
			MS_DEBUG_DEV("</PayloadDescriptor>");
		}

		VP9::PayloadDescriptorHandler::PayloadDescriptorHandler(VP9::PayloadDescriptor* payloadDescriptor)
		{
			MS_TRACE();

			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool VP9::PayloadDescriptorHandler::Process(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* /*data*/, bool& marker)
		{
			MS_TRACE();

			auto* context = static_cast<RTC::Codecs::VP9::EncodingContext*>(encodingContext);

			MS_ASSERT(context->GetTargetSpatialLayer() >= 0, "target spatial layer cannot be -1");
			MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

			// If the flag L is not set, drop the packet.
			if (!this->payloadDescriptor->l)
				return false;

			// If packet spatial or temporal layer is higher than maximum
			// announced one, drop the packet.
			// clang-format off
			if (
				GetSpatialLayer() >= context->GetSpatialLayers() ||
				GetTemporalLayer() >= context->GetTemporalLayers()
			)
			// clang-format on
			{
				return false;
			}

			auto currentSpatialLayer  = context->GetCurrentSpatialLayer();
			auto currentTemporalLayer = context->GetCurrentTemporalLayer();

			// MS_ERROR("GetSpatialLayer():%d, context->GetCurrentSpatialLayer():%d, context->GetTargetSpatialLayer:%d"
				 // ", GetTemporalLayer():%d, context->GetCurrentTemporalLayer():%d, context->GetTargetTemporalLayer:%d",
				// GetSpatialLayer(), context->GetCurrentSpatialLayer(), context->GetTargetSpatialLayer(),
				// GetTemporalLayer(), context->GetCurrentTemporalLayer(), context->GetTargetTemporalLayer());

			// MS_ERROR("p:%d|b:%d",
					// this->payloadDescriptor->p,
					// this->payloadDescriptor->b);

			// Upgrade current spatial layer if needed.
			// clang-format off
			if (
				context->GetTargetSpatialLayer() > context->GetCurrentSpatialLayer() &&
				GetSpatialLayer() > context->GetCurrentSpatialLayer() &&
				GetSpatialLayer() <= context->GetTargetSpatialLayer()
			)
			// clang-format on
			{
				/*
				 * Upgrade spatial layer if:
				 *
				 * Inter-picture predicted frame equals zero.
				 * Beginning of a frame.
				 */
				if (!this->payloadDescriptor->p && this->payloadDescriptor->b)
				{
					// Update current spatial layer.
					currentSpatialLayer = GetSpatialLayer();
				}
				else
				{
					return false;
				}
			}

			// Downgrade current spatial layer if needed.
			// clang-format off
			else if (
				context->GetTargetSpatialLayer() < context->GetCurrentSpatialLayer() &&
				GetSpatialLayer() < context->GetCurrentSpatialLayer() &&
				GetSpatialLayer() >= context->GetTargetSpatialLayer()
			)
			// clang-format on
			{
				/*
				 * Downgrade spatial layer if:
				 *
				 * End of a frame.
				 */
				if (this->payloadDescriptor->e)
					currentSpatialLayer = GetSpatialLayer();
			}

			// Update current temporal layer if needed.
			// (just if the packet spatial layer equals current spatial layer).
			if (GetSpatialLayer() == currentSpatialLayer)
			{
				// Upgrade current temporal layer if needed.
				// clang-format off
				if (
						context->GetTargetTemporalLayer() > context->GetCurrentTemporalLayer() &&
						GetTemporalLayer() > context->GetCurrentTemporalLayer() &&
						GetTemporalLayer() <= context->GetTargetTemporalLayer()
				)
				// clang-format on
				{
					/*
					 * Upgrade temporal layer if:
					 *
					 * 'Switching up point' bit is set.
					 * Beginning of a frame.
					 */
					// clang-format off
					if (
							(
							 context->GetCurrentTemporalLayer() == -1 ||
							 this->payloadDescriptor->switchingUpPoint
							) &&
							this->payloadDescriptor->b
						 )
					// clang-format on
					{
						// Update current temporal layer.
						currentTemporalLayer = GetTemporalLayer();
					}
				}

				// Downgrade current temporal layer if needed.
				// clang-format off
				else if (
						context->GetTargetTemporalLayer() < context->GetCurrentTemporalLayer() &&
						GetTemporalLayer() < context->GetCurrentTemporalLayer() &&
						GetTemporalLayer() >= context->GetTargetTemporalLayer()
				)
				// clang-format on
				{
					/*
					 * Downgrade temporal layer if:
					 *
					 * End of a frame.
					 */
					if (this->payloadDescriptor->e)
					{
						// Update current temporal layer.
						currentTemporalLayer = GetTemporalLayer();
					}
				}
			}

			// Filter spatial layers higher than current one.
			if (GetSpatialLayer() > currentSpatialLayer)
				return false;

			// Filter temporal layers higher than current one.
			if (GetTemporalLayer() > currentTemporalLayer)
				return false;

			// Set marker bit if needed.
			// clang-format off
			if (
				GetSpatialLayer() == currentSpatialLayer &&
				this->payloadDescriptor->e
			)
			// clang-format on
			{
				// Set RTP marker bit.
				marker = true;
			}

			// Update current spatial layer if needed.
			if (currentSpatialLayer != context->GetCurrentSpatialLayer())
				context->SetCurrentSpatialLayer(currentSpatialLayer);

			// Update current temporal layer if needed.
			if (currentTemporalLayer != context->GetCurrentTemporalLayer())
				context->SetCurrentTemporalLayer(currentTemporalLayer);

			return true;
		}

		void VP9::PayloadDescriptorHandler::Restore(uint8_t* /*data*/)
		{
			MS_TRACE();
		}
	} // namespace Codecs
} // namespace RTC
