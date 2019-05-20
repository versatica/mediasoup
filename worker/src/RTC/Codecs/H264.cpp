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
		  const uint8_t* data,
		  size_t len,
		  RTC::RtpPacket::FrameMarking* frameMarking,
		  uint8_t /*frameMarkingLen*/)
		{
			MS_TRACE();

			if (len < 2)
				return nullptr;

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			// Use frame-marking.
			if (frameMarking)
			{
				// Detect key frame.
				if (frameMarking->start && frameMarking->independent)
					payloadDescriptor->isKeyFrame = true;

				// TODO: Read frameMarking->tid (temporal layer).
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
			MS_DEBUG_DEV("  isKeyFrame : %s", this->isKeyFrame ? "true" : "false");
			MS_DEBUG_DEV("</PayloadDescriptor>");
		}

		H264::PayloadDescriptorHandler::PayloadDescriptorHandler(H264::PayloadDescriptor* payloadDescriptor)
		{
			this->payloadDescriptor.reset(payloadDescriptor);
		}
	} // namespace Codecs
} // namespace RTC
