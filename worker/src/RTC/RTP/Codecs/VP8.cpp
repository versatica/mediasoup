#define MS_CLASS "RTC::RTP::Codecs::VP8"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Codecs/VP8.hpp"
#include "Logger.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptorPayloadDescriptor.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/* Class methods. */

			VP8::PayloadDescriptor* VP8::Parse(const uint8_t* data, size_t len)
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

				payloadDescriptor->extended       = (byte >> 7) & 0x01;
				payloadDescriptor->nonReference   = (byte >> 5) & 0x01;
				payloadDescriptor->start          = (byte >> 4) & 0x01;
				payloadDescriptor->partitionIndex = byte & 0x07;

				if (payloadDescriptor->extended)
				{
					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (2)");

						return nullptr;
					}

					byte = data[offset];

					payloadDescriptor->i = (byte >> 7) & 0x01;
					payloadDescriptor->l = (byte >> 6) & 0x01;
					payloadDescriptor->t = (byte >> 5) & 0x01;
					payloadDescriptor->k = (byte >> 4) & 0x01;
				}

				if (payloadDescriptor->i)
				{
					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (3)");

						return nullptr;
					}

					byte = data[offset];

					if ((byte >> 7) & 0x01)
					{
						if (len < ++offset + 1)
						{
							MS_WARN_DEV("ignoring invalid payload (4)");

							return nullptr;
						}

						payloadDescriptor->hasTwoBytesPictureId = true;
						payloadDescriptor->pictureId            = (byte & 0x7F) << 8;
						payloadDescriptor->pictureId += data[offset];
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
						MS_WARN_DEV("ignoring invalid payload (5)");

						return nullptr;
					}

					payloadDescriptor->hasTl0PictureIndex = true;
					payloadDescriptor->tl0PictureIndex    = data[offset];
				}

				if (payloadDescriptor->t || payloadDescriptor->k)
				{
					if (len < ++offset + 1)
					{
						MS_WARN_DEV("ignoring invalid payload (6)");

						return nullptr;
					}

					byte = data[offset];

					payloadDescriptor->hasTlIndex = true;
					payloadDescriptor->tlIndex    = (byte >> 6) & 0x03;
					payloadDescriptor->y          = (byte >> 5) & 0x01;
					payloadDescriptor->keyIndex   = byte & 0x1F;
				}

				if (
				  // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
				  (len >= ++offset + 1) && payloadDescriptor->start &&
				  payloadDescriptor->partitionIndex == 0 && (!(data[offset] & 0x01)) // Inverse Keyframe bit.
				)
				{
					payloadDescriptor->isKeyFrame = true;
				}

				return payloadDescriptor.release();
			}

			bool VP8::ProcessRtpPacket(
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
					auto* const dependencyDescriptorPayloadDescriptor =
					  DependencyDescriptorPayloadDescriptor::Parse(packet, dependencyDescriptorContext);

					// The descriptor could not be parsed.
					if (!dependencyDescriptorPayloadDescriptor)
					{
						return false;
					}

					packet->SetDependencyDescriptorPayloadDescriptorHandler(
					  std::make_unique<DependencyDescriptorPayloadDescriptorHandler>(
					    dependencyDescriptorPayloadDescriptor));
				}

				auto* data = packet->GetPayload();
				auto len   = packet->GetPayloadLength();

				std::unique_ptr<PayloadDescriptor> payloadDescriptor{ VP8::Parse(data, len) };

				if (!payloadDescriptor)
				{
					return true;
				}

				if (payloadDescriptor->hasPictureId)
				{
					// Extend the pictureId into the 15 bits, that is, carrying the cycles a 7
					// bits one has already wrapped.
					//
					// NOTE: The field is widened below, which hides the width the sender
					// used, so the extended value is written into it. A pipe Consumer does
					// not rewrite the pictureId, so the worker at the other end would
					// otherwise be given a 15 bits field holding a value that wraps every 128
					// pictures.
					const auto pictureId = pictureIdUnwrapper.Unwrap(
					  payloadDescriptor->pictureId, payloadDescriptor->hasTwoBytesPictureId);

					// Modify the RtpPacket payload in order to always have two byte
					// pictureId, which is what an extended one takes and what `Encode()`
					// writes into it.
					if (!payloadDescriptor->hasTwoBytesPictureId)
					{
						// Shift the RTP payload one byte from the begining of the pictureId
						// field. The packet is not forwarded if it has no room for it, since
						// every Consumer writes a 15 bits pictureId into it from here on.
						if (!packet->ShiftPayload(2, 1))
						{
							return false;
						}

						// Set the two byte pictureId marker bit.
						data[2] = 0x80;
					}

					// Write the extended pictureId if it changed, which happens if a 7 bits
					// one has already wrapped or a sender has just widened it to 15 bits.
					if (pictureId != payloadDescriptor->pictureId)
					{
						payloadDescriptor->Encode(data, pictureId, payloadDescriptor->tl0PictureIndex);

						payloadDescriptor->pictureId = pictureId;
					}
				}

				// NOTE: Set once the packet carries what the descriptor says, so that a
				// packet that could not be widened is left with none.
				packet->SetPayloadDescriptorHandler(
				  std::make_unique<PayloadDescriptorHandler>(payloadDescriptor.release()));

				return true;
			}

			/* Instance methods. */

			void VP8::PayloadDescriptor::Dump(int indentation) const
			{
				MS_TRACE();

				MS_DUMP_CLEAN(indentation, "<VP8::PayloadDescriptor>");
				MS_DUMP_CLEAN(
				  indentation,
				  "  i:%" PRIu8 "|l:%" PRIu8 "|t:%" PRIu8 "|k:%" PRIu8,
				  this->i,
				  this->l,
				  this->t,
				  this->k);
				MS_DUMP_CLEAN(indentation, "  extended: %" PRIu8, this->extended);
				MS_DUMP_CLEAN(indentation, "  nonReference: %" PRIu8, this->nonReference);
				MS_DUMP_CLEAN(indentation, "  start: %" PRIu8, this->start);
				MS_DUMP_CLEAN(indentation, "  partitionIndex: %" PRIu8, this->partitionIndex);
				MS_DUMP_CLEAN(indentation, "  pictureId: %" PRIu16, this->pictureId);
				MS_DUMP_CLEAN(indentation, "  tl0PictureIndex: %" PRIu8, this->tl0PictureIndex);
				MS_DUMP_CLEAN(indentation, "  tlIndex: %" PRIu8, this->tlIndex);
				MS_DUMP_CLEAN(indentation, "  y: %" PRIu8, this->y);
				MS_DUMP_CLEAN(indentation, "  keyIndex: %" PRIu8, this->keyIndex);
				MS_DUMP_CLEAN(indentation, "  isKeyFrame: %s", this->isKeyFrame ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  hasPictureId: %s", this->hasPictureId ? "true" : "false");
				MS_DUMP_CLEAN(
				  indentation, "  hasTwoBytesPictureId: %s", this->hasTwoBytesPictureId ? "true" : "false");
				MS_DUMP_CLEAN(
				  indentation, "  hasTl0PictureIndex: %s", this->hasTl0PictureIndex ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  hasTlIndex: %s", this->hasTlIndex ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "</VP8::PayloadDescriptor>");
			}

			void VP8::PayloadDescriptor::Encode(uint8_t* data, uint16_t pictureId, uint8_t tl0PictureIndex) const
			{
				MS_TRACE();

				if (data == nullptr)
				{
					MS_ABORT("data is nullptr");
				}

				// Nothing to do.
				if (!this->extended)
				{
					return;
				}

				data += 2;

				if (this->i)
				{
					uint16_t netPictureId = htons(pictureId);

					std::memcpy(data, std::addressof(netPictureId), 2);
					data[0] |= 0x80;
					data += 2;
				}

				if (this->l)
				{
					*data = tl0PictureIndex;
				}
			}

			void VP8::PayloadDescriptor::Restore(uint8_t* data) const
			{
				MS_TRACE();

				if (this->hasPictureId && this->hasTl0PictureIndex)
				{
					Encode(data, this->pictureId, this->tl0PictureIndex);
				}
			}

			void VP8::PayloadDescriptor::Encoder::Encode(
			  uint8_t* data, const PayloadDescriptor* payloadDescriptor) const
			{
				MS_TRACE();

				payloadDescriptor->Encode(
				  data, this->encodingData.pictureId, this->encodingData.tl0PictureIndex);
			}

			VP8::PayloadDescriptorHandler::PayloadDescriptorHandler(VP8::PayloadDescriptor* payloadDescriptor)
			{
				MS_TRACE();

				this->payloadDescriptor.reset(payloadDescriptor);
			}

			bool VP8::PayloadDescriptorHandler::Process(
			  Codecs::EncodingContext* encodingContext, RTP::Packet* packet, bool& /*marker*/)
			{
				MS_TRACE();

				auto* context = static_cast<Codecs::VP8::EncodingContext*>(encodingContext);

				MS_ASSERT(context->GetTargetTemporalLayer() >= 0, "target temporal layer cannot be -1");

				// Check if the payload should contain temporal layer info.
				if (context->GetTemporalLayers() > 1 && !this->payloadDescriptor->hasTlIndex)
				{
					MS_WARN_DEV("stream is supposed to have >1 temporal layers but does not have TlIndex field");
				}

				// NOTE: `ProcessRtpPacket()` already extended the pictureId into the 15
				// bits, so it is used as it comes from here on.

				// Check whether pictureId and tl0PictureIndex sync is required.
				if (
				  context->syncRequired && this->payloadDescriptor->hasPictureId &&
				  this->payloadDescriptor->hasTl0PictureIndex)
				{
					context->pictureIdManager.Sync(this->payloadDescriptor->pictureId - 1);
					context->tl0PictureIndexManager.Sync(this->payloadDescriptor->tl0PictureIndex - 1);

					context->syncRequired = false;
				}

				// Incremental pictureId. Check the temporal layer.
				if (
				  this->payloadDescriptor->hasPictureId && this->payloadDescriptor->hasTlIndex &&
				  this->payloadDescriptor->hasTl0PictureIndex &&
				  !RTC::SeqManager<uint16_t, VP8::FrameIdLength>::IsSeqLowerThan(
				    this->payloadDescriptor->pictureId, context->pictureIdManager.GetMaxInput()))
				{
					// Drop if:
					// - Temporal layer is higher than target.
					// - Temporal layer is higher than current and sync flag is not set.
					if (
					  std::cmp_greater(this->payloadDescriptor->tlIndex, context->GetTargetTemporalLayer()) ||
					  (std::cmp_greater(this->payloadDescriptor->tlIndex, context->GetCurrentTemporalLayer()) &&
						 !this->payloadDescriptor->y))
					{
						context->pictureIdManager.Drop(this->payloadDescriptor->pictureId);

						if (this->payloadDescriptor->tlIndex == 0)
						{
							context->tl0PictureIndexManager.Drop(this->payloadDescriptor->tl0PictureIndex);
						}

						return false;
					}
				}

				// Update pictureId and tl0PictureIndex values.
				uint16_t pictureId;
				uint8_t tl0PictureIndex;

				// Do not send a dropped pictureId.
				if (
				  this->payloadDescriptor->hasPictureId &&
				  !context->pictureIdManager.Input(this->payloadDescriptor->pictureId, pictureId))
				{
					return false;
				}

				// Do not send a dropped tl0PictureIndex.
				if (
				  this->payloadDescriptor->hasTl0PictureIndex &&
				  !context->tl0PictureIndexManager.Input(
				    this->payloadDescriptor->tl0PictureIndex, tl0PictureIndex))
				{
					return false;
				}

				// VP8 always has a single spatial layer (0).
				context->SetCurrentSpatialLayer(0);

				// Update/fix current temporal layer.
				if (
				  this->payloadDescriptor->hasTlIndex &&
				  std::cmp_equal(this->payloadDescriptor->tlIndex, context->GetTargetTemporalLayer()))
				{
					context->SetCurrentTemporalLayer(this->payloadDescriptor->tlIndex);
				}
				else if (!this->payloadDescriptor->hasTlIndex)
				{
					context->SetCurrentTemporalLayer(0);
				}

				if (context->GetCurrentTemporalLayer() > context->GetTargetTemporalLayer())
				{
					context->SetCurrentTemporalLayer(context->GetTargetTemporalLayer());
				}

				// Do not send tlIndex higher than current one.
				if (std::cmp_greater(this->payloadDescriptor->tlIndex, context->GetCurrentTemporalLayer()))
				{
					return false;
				}

				if (this->payloadDescriptor->hasPictureId && this->payloadDescriptor->hasTl0PictureIndex)
				{
					this->payloadDescriptor->Encode(packet->GetPayload(), pictureId, tl0PictureIndex);

					// Leave the Consumer how this packet was rewritten for it.
					auto encoder =
					  std::make_unique<VP8::PayloadDescriptor::Encoder>(VP8::PayloadDescriptor::EncodingData{
					    .pictureId = pictureId, .tl0PictureIndex = tl0PictureIndex });

					encoder->preferDependencyDescriptor = context->PrefersDependencyDescriptor();

					context->encoder = std::move(encoder);
				}

				return true;
			};

			void VP8::PayloadDescriptorHandler::Encode(
			  RTP::Packet* packet, Codecs::PayloadDescriptor::Encoder* encoder)
			{
				MS_TRACE();

				auto* vp8Encoder = static_cast<VP8::PayloadDescriptor::Encoder*>(encoder);

				vp8Encoder->Encode(packet->GetPayload(), this->payloadDescriptor.get());
			}

			void VP8::PayloadDescriptorHandler::Restore(RTP::Packet* packet)
			{
				MS_TRACE();

				if (this->payloadDescriptor->hasPictureId && this->payloadDescriptor->hasTl0PictureIndex)
				{
					this->payloadDescriptor->Restore(packet->GetPayload());
				}
			}
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC
