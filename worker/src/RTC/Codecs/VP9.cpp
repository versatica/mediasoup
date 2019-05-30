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
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* /*data*/)
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

			// Filter spatial layers higher than targeted one.
			if (GetSpatialLayer() > context->GetTargetSpatialLayer())
				return false;

			// Update current spatial layer if needed.
			if (GetSpatialLayer() > context->GetCurrentSpatialLayer())
			{
				// TODO: Check if we can upgrade spatial layer.

				if (GetSpatialLayer() == context->GetTargetSpatialLayer())
				{
					// NOTE: This may be wrong since packet may have temporal layer 1 while
					// target temporal may be 2, so we are setting current temporal layer to
					// 2 instead of 1. However, if we are here it may mean that the packet
					// satisfies "any" temporal layer so we are done. Who knows.
					context->SetCurrentLayers(GetSpatialLayer(), context->GetTargetTemporalLayer());
				}
				else
				{
					context->SetCurrentLayers(GetSpatialLayer(), context->GetTemporalLayers() - 1);
				}

				return true;
			}

			// If packet spatial layer equals target spatial layer and packet temporal
			// layer higher than target, drop the packet.
			// clang-format off
			if (
				GetSpatialLayer() == context->GetTargetSpatialLayer() &&
				GetTemporalLayer() > context->GetTargetTemporalLayer()
			)
			// clang-format on
			{
				return false;
			}

			// Packet temporal layer higher than current, update current temporal
			// layer if needed (just if the packet spatial layer equals current spatial
			// layer).
			// clang-format off
			if (
				GetSpatialLayer() == context->GetCurrentSpatialLayer() &&
				GetTemporalLayer() > context->GetCurrentTemporalLayer()
			)
			// clang-format on
			{
				// Upgrade if 'Switching up point' bit is set.
				if (this->payloadDescriptor->switchingUpPoint)
				{
					context->SetCurrentLayers(context->GetCurrentSpatialLayer(), GetTemporalLayer());

					return true;
				}
				else
				{
					return false;
				}
			}

			return true;
		}

		void VP9::PayloadDescriptorHandler::Restore(uint8_t* /*data*/)
		{
			MS_TRACE();
		}
	} // namespace Codecs
} // namespace RTC
