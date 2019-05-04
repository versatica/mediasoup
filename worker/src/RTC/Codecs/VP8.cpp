#define MS_CLASS "RTC::Codecs::VP8"
// #define MS_LOG_DEV

#include "RTC/Codecs/VP8.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace Codecs
	{
		VP8::PayloadDescriptor* VP8::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			if (len < 1)
				return nullptr;

			size_t offset{ 0 };
			uint8_t byte = data[offset];

			payloadDescriptor->extended       = (byte >> 7) & 0x01;
			payloadDescriptor->nonReference   = (byte >> 5) & 0x01;
			payloadDescriptor->start          = (byte >> 4) & 0x01;
			payloadDescriptor->partitionIndex = byte & 0x07;

			if (!payloadDescriptor->extended)
			{
				return nullptr;
			}
			else
			{
				if (len < ++offset + 1)
					return nullptr;

				byte = data[offset];

				payloadDescriptor->i = (byte >> 7) & 0x01;
				payloadDescriptor->l = (byte >> 6) & 0x01;
				payloadDescriptor->t = (byte >> 5) & 0x01;
				payloadDescriptor->k = (byte >> 4) & 0x01;
			}

			if (payloadDescriptor->i)
			{
				if (len < ++offset + 1)
					return nullptr;

				byte = data[offset];

				if ((byte >> 7) & 0x01)
				{
					if (len < ++offset + 1)
						return nullptr;

					payloadDescriptor->hasTwoBytesPictureId = true;
					payloadDescriptor->pictureId            = (byte & 0x7F) << 8;
					payloadDescriptor->pictureId += data[offset];
				}
				else
				{
					payloadDescriptor->hasOneBytePictureId = true;
					payloadDescriptor->pictureId           = byte & 0x7F;
				}

				payloadDescriptor->hasPictureId = true;
			}

			if (payloadDescriptor->l)
			{
				if (len < ++offset + 1)
					return nullptr;

				payloadDescriptor->hasTl0PictureIndex = true;
				payloadDescriptor->tl0PictureIndex    = data[offset];
			}

			if (payloadDescriptor->t || payloadDescriptor->k)
			{
				if (len < ++offset + 1)
					return nullptr;

				byte = data[offset];

				payloadDescriptor->hasTlIndex = true;
				payloadDescriptor->tlIndex    = (byte >> 6) & 0x03;
				payloadDescriptor->y          = (byte >> 5) & 0x01;
				payloadDescriptor->keyIndex   = byte & 0x1F;
			}

			if (
			  (len >= ++offset + 1) && payloadDescriptor->start &&
			  payloadDescriptor->partitionIndex == 0 && (!(data[offset] & 0x01)))
			{
				payloadDescriptor->isKeyFrame = true;
			}

			return payloadDescriptor.release();
		}

		void VP8::PayloadDescriptor::Encode(uint8_t* data, uint16_t pictureId, uint8_t tl0PictureIndex) const
		{
			MS_TRACE();

			// Nothing to do.
			if (!this->extended)
				return;

			data += 2;

			if (this->i)
			{
				if (this->hasTwoBytesPictureId)
				{
					uint16_t netPictureId = htons(pictureId);

					std::memcpy(data, &netPictureId, 2);
					data[0] |= 0x80;
					data += 2;
				}
				else if (this->hasOneBytePictureId)
				{
					*data = pictureId;
					data++;

					if (pictureId > 127)
						MS_DEBUG_TAG(rtp, "casting pictureId value to one byte");
				}
			}

			if (this->l)
				*data = tl0PictureIndex;
		}

		void VP8::PayloadDescriptor::Restore(uint8_t* data) const
		{
			this->Encode(data, this->pictureId, this->tl0PictureIndex);
		}

		void VP8::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<PayloadDescriptor>");
			MS_DEBUG_DEV("  extended        : %" PRIu8, this->extended);
			MS_DEBUG_DEV("  nonReference    : %" PRIu8, this->nonReference);
			MS_DEBUG_DEV("  start           : %" PRIu8, this->start);
			MS_DEBUG_DEV("  partitionIndex  : %" PRIu8, this->partitionIndex);
			MS_DEBUG_DEV(
			  "  i|l|t|k         : %" PRIu8 "|%" PRIu8 "|%" PRIu8 "|%" PRIu8,
			  this->i,
			  this->l,
			  this->t,
			  this->k);
			MS_DEBUG_DEV("  pictureId       : %" PRIu16, this->pictureId);
			MS_DEBUG_DEV("  tl0PictureIndex : %" PRIu8, this->tl0PictureIndex);
			MS_DEBUG_DEV("  tlIndex         : %" PRIu8, this->tlIndex);
			MS_DEBUG_DEV("  y               : %" PRIu8, this->y);
			MS_DEBUG_DEV("  keyIndex        : %" PRIu8, this->keyIndex);
			MS_DEBUG_DEV("  isKeyFrame      : %s", this->isKeyFrame ? "true" : "false");
			MS_DEBUG_DEV("  hasPictureId    : %s", this->hasPictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasOneBytePictureId  : %s", this->hasOneBytePictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasTwoBytesPictureId : %s", this->hasTwoBytesPictureId ? "true" : "false");
			MS_DEBUG_DEV("  hasTl0PictureIndex   : %s", this->hasTl0PictureIndex ? "true" : "false");
			MS_DEBUG_DEV("  hasTlIndex           : %s", this->hasTlIndex ? "true" : "false");
			MS_DEBUG_DEV("</PayloadDescriptor>");
		}

		VP8::PayloadDescriptorHandler::PayloadDescriptorHandler(VP8::PayloadDescriptor* payloadDescriptor)
		{
			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool VP8::PayloadDescriptorHandler::Encode(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* data)
		{
			auto* context = static_cast<EncodingContext*>(encodingContext);

			// Check whether pictureId and tl0PictureIndex sync is required.
			if (context->syncRequired)
			{
				context->pictureIdManager.Sync(this->payloadDescriptor->pictureId);
				context->tl0PictureIndexManager.Sync(this->payloadDescriptor->tl0PictureIndex);
				context->syncRequired = false;
			}

			// Incremental pictureId. Check the temporal layer.
			if (
			  this->payloadDescriptor->hasPictureId && this->payloadDescriptor->hasTlIndex &&
			  this->payloadDescriptor->hasTl0PictureIndex)
			{
				if (RTC::SeqManager<uint16_t>::IsSeqHigherThan(
				      this->payloadDescriptor->pictureId, context->pictureIdManager.GetMaxInput()))
				{
					if (this->payloadDescriptor->tlIndex > context->preferences.temporalLayer)
					{
						context->pictureIdManager.Drop(this->payloadDescriptor->pictureId);
						context->tl0PictureIndexManager.Drop(this->payloadDescriptor->tl0PictureIndex);

						return false;
					}
					else if (context->currentTemporalLayer != context->preferences.temporalLayer)
					{
						// Payload descriptor contains the target temporal layer.
						if (this->payloadDescriptor->tlIndex == context->preferences.temporalLayer)
						{
							// Upgrade required.
							if (context->currentTemporalLayer < this->payloadDescriptor->tlIndex)
							{
								// Drop current packet if sync flag is not set.
								if (this->payloadDescriptor->y != 1u)
								{
									context->pictureIdManager.Drop(this->payloadDescriptor->pictureId);
									context->tl0PictureIndexManager.Drop(this->payloadDescriptor->tl0PictureIndex);

									return false;
								}
							}

							context->currentTemporalLayer = this->payloadDescriptor->tlIndex;
						}
					}
				}
			}

			// Update pictureId and tl0PictureIndex values.
			uint16_t pictureId;
			uint8_t tl0PictureIndex;

			// Do not send a dropped pictureId.
			if (!context->pictureIdManager.Input(this->payloadDescriptor->pictureId, pictureId))
				return false;

			// Do not send a dropped tl0PicutreIndex.
			if (!context->tl0PictureIndexManager.Input(
			      this->payloadDescriptor->tl0PictureIndex, tl0PictureIndex))
			{
				return false;
			}

			this->payloadDescriptor->Encode(data, pictureId, tl0PictureIndex);

			return true;
		};

		void VP8::PayloadDescriptorHandler::Restore(uint8_t* data)
		{
			this->payloadDescriptor->Restore(data);
		}

		void VP8::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto data = packet->GetPayload();
			auto len  = packet->GetPayloadLength();

			PayloadDescriptor* payloadDescriptor = Parse(data, len);

			if (!payloadDescriptor)
				return;

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);

			// Modify the RtpPacket payload in order to always have two byte pictureId.
			if (payloadDescriptor->hasOneBytePictureId)
			{
				// Shift the RTP payload one byte from the begining of the pictureId field.
				packet->ShiftPayload(2, 1, true /*expand*/);

				// Set the two byte pictureId marker bit.
				data[2] = 0x80;

				// Update the payloadDescriptor.
				payloadDescriptor->hasOneBytePictureId  = false;
				payloadDescriptor->hasTwoBytesPictureId = true;
			}
		}
	} // namespace Codecs
} // namespace RTC
