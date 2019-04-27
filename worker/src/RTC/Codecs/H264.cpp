#define MS_CLASS "RTC::Codecs::H264"
// #define MS_LOG_DEV

#include "RTC/Codecs/H264.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		H264::PayloadDescriptor* H264::Parse(uint8_t* data, size_t len)
		{
			MS_TRACE();

			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			if (len < 2)
				return nullptr;

			uint8_t nal = *data & 0x1F;

			switch (nal)
			{
				// Single NAL unit packet.
				// IDR (instantaneous decoding picture).
				case 7:
				{
					MS_ERROR("H264: ***KEYFRAME*** [nal:7]");

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

						if (subnal == 7 || subnal == 8)
						{
							MS_ERROR("H264: ***KEYFRAME*** [nal:24, subnal:%" PRIu8 "]", subnal);

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
					uint8_t subnal  = *(data + 1) & 0x1F;
					uint8_t startBit = *(data + 1) & 0x80;

					if ((subnal == 7 || subnal == 8) && startBit == 128)
					{
						MS_ERROR("H264: ***KEYFRAME*** [nal:%" PRIu8 ", subnal:%" PRIu8 ", startBit:%" PRIu8 "]", nal, subnal, startBit);

						payloadDescriptor->isKeyFrame = true;
					}

					break;
				}
			}

			return payloadDescriptor.release();
		}

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

		void H264::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto data = packet->GetPayload();
			auto len  = packet->GetPayloadLength();

			PayloadDescriptor* payloadDescriptor = Parse(data, len);

			if (!payloadDescriptor)
				return;

			auto* payloadDescriptorHandler = new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}
	} // namespace Codecs
} // namespace RTC
