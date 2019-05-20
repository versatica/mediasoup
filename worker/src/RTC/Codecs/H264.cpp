#define MS_CLASS "RTC::Codecs::H264"
// #define MS_LOG_DEV

#include "RTC/Codecs/H264.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		/* Class methods. */

		H264::PayloadDescriptor* H264::Parse(
		  const uint8_t* data, size_t len, RTC::RtpPacket::FrameMarking* frameMarking, uint8_t frameMarkingLen)
		{
			MS_TRACE();

			if (len < 2)
				return nullptr;

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			// Use frame-marking.
			if (frameMarking)
			{
				// Read fields.
				payloadDescriptor->s   = frameMarking->start;
				payloadDescriptor->e   = frameMarking->end;
				payloadDescriptor->i   = frameMarking->independent;
				payloadDescriptor->d   = frameMarking->discardable;
				payloadDescriptor->b   = frameMarking->base;
				payloadDescriptor->tid = frameMarking->tid;

				if (frameMarkingLen >= 2)
				{
					payloadDescriptor->hasLid = true;
					payloadDescriptor->lid    = frameMarking->lid;
				}

				if (frameMarkingLen == 3)
				{
					payloadDescriptor->hasTl0picidx = true;
					payloadDescriptor->tl0picidx    = frameMarking->tl0picidx;
				}

				// Detect key frame.
				if (frameMarking->start && frameMarking->independent)
					payloadDescriptor->isKeyFrame = true;
			}
			else
			// Parse payload otherwise.
			{
				uint8_t nal = *data & 0x1F;

				switch (nal)
				{
					// Single NAL unit packet.
					// IDR (instantaneous decoding picture).
					case 7:
					{
						payloadDescriptor->isKeyFrame = true;

						break;
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
							auto naluSize  = Utils::Byte::Get2Bytes(data, offset);
							uint8_t subnal = *(data + offset + sizeof(naluSize)) & 0x1F;

							if (subnal == 7)
							{
								payloadDescriptor->isKeyFrame = true;

								break;
							}

							// Check if there is room for the indicated NAL unit size.
							if (len < (naluSize + sizeof(naluSize)))
								break;

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
						uint8_t subnal   = *(data + 1) & 0x1F;
						uint8_t startBit = *(data + 1) & 0x80;

						if (subnal == 7 && startBit == 128)
							payloadDescriptor->isKeyFrame = true;

						break;
					}
				}
			}

			return payloadDescriptor.release();
		}

		void H264::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto* data = packet->GetPayload();
			auto len   = packet->GetPayloadLength();
			RtpPacket::FrameMarking* frameMarking{ nullptr };
			uint8_t frameMarkingLen{ 0 };

			// Read frame-marking.
			packet->ReadFrameMarking(&frameMarking, frameMarkingLen);

			PayloadDescriptor* payloadDescriptor = H264::Parse(data, len, frameMarking, frameMarkingLen);

			if (!payloadDescriptor)
				return;

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}

		/* Instance methods. */

		void H264::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<PayloadDescriptor>");
			MS_DEBUG_DEV("  s          : %s", this->s ? "true" : "false");
			MS_DEBUG_DEV("  e          : %s", this->e ? "true" : "false");
			MS_DEBUG_DEV("  i          : %s", this->i ? "true" : "false");
			MS_DEBUG_DEV("  d          : %s", this->d ? "true" : "false");
			MS_DEBUG_DEV("  b          : %s", this->b ? "true" : "false");
			MS_DEBUG_DEV("  tid        : %" PRIu8, this->tid);
			if (this->hasLid)
				MS_DEBUG_DEV("  lid        : %" PRIu8, this->lid);
			if (this->hasTl0picidx)
				MS_DEBUG_DEV("  tl0picidx  : %" PRIu8, this->tl0picidx);
			MS_DEBUG_DEV("  isKeyFrame : %s", this->isKeyFrame ? "true" : "false");
			MS_DEBUG_DEV("</PayloadDescriptor>");
		}

		H264::PayloadDescriptorHandler::PayloadDescriptorHandler(H264::PayloadDescriptor* payloadDescriptor)
		{
			MS_TRACE();

			this->payloadDescriptor.reset(payloadDescriptor);
		}

		bool H264::PayloadDescriptorHandler::Encode(
		  RTC::Codecs::EncodingContext* encodingContext, uint8_t* /*data*/)
		{
			MS_TRACE();

			auto* context = static_cast<RTC::Codecs::H264::EncodingContext*>(encodingContext);

			// If a key frame, update currentTemporalLayer.
			if (this->payloadDescriptor->isKeyFrame)
				context->currentTemporalLayer = std::numeric_limits<uint8_t>::max();

			// TODO: We should check incremental picture id here somehow and let it pass
			// if older than highest seen.

			// Check temporal layer.
			if (this->payloadDescriptor->tid > context->preferences.temporalLayer)
			{
				return false;
			}
			// Upgrade required. Drop current packet if base flag is not set.
			// TODO: Cannot enable this until this issue is fixed (in libwebrtc?):
			//   https://github.com/versatica/mediasoup/issues/306
			//
			// clang-format off
			// else if (
			// 	this->payloadDescriptor->tid > context->currentTemporalLayer &&
			// 	!this->payloadDescriptor->b
			// )
			// // clang-format on
			// {
			// 	return false;
			// }

			// Update/fix currentTemporalLayer.
			if (this->payloadDescriptor->tid > context->currentTemporalLayer)
				context->currentTemporalLayer = this->payloadDescriptor->tid;
			else if (context->currentTemporalLayer > context->preferences.temporalLayer)
				context->currentTemporalLayer = context->preferences.temporalLayer;

			return true;
		}

		void H264::PayloadDescriptorHandler::Restore(uint8_t* /*data*/)
		{
			MS_TRACE();
		}
	} // namespace Codecs
} // namespace RTC
