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

				// TODO <TPM>
				uint8_t kk_nal = *data & 0x1F;

				if (kk_nal != 1 && kk_nal != 28)
					MS_ERROR("    *** [nal:%" PRIu8 "]", kk_nal);

			switch (kk_nal)
			{
				// Single NAL unit packet.
				// IDR (instantaneous decoding picture).
				case 7:
				{
					MS_ERROR("<H264>");
					MS_ERROR("  [nal:7]");
					MS_ERROR("</H264>");

					break;
				}

				// Aggreation packet.
				// STAP-A.
				case 24:
				{
					MS_ERROR("<H264>");
					MS_ERROR("  [nal:24]");

					size_t offset{ 1 };

					len -= 1;

					// Iterate NAL units.
					while (len >= 3)
					{
						auto naluSize  = Utils::Byte::Get2Bytes(data, offset);
						uint8_t kk_subnal = *(data + offset + sizeof(naluSize)) & 0x1F;

						MS_ERROR("    [subnal:%" PRIu8 "]", kk_subnal);

						// Check if there is room for the indicated NAL unit size.
						if (len < (naluSize + sizeof(naluSize)))
							break;

						offset += naluSize + sizeof(naluSize);
						len -= naluSize + sizeof(naluSize);
					}

					MS_ERROR("</H264>");

					break;
				}

				// Aggreation packet.
				// FU-A, FU-B.
				case 28:
				case 29:
				{
					uint8_t subnal   = *(data + 1) & 0x1F;
					uint8_t startBit = *(data + 1) & 0x80;

					if (kk_nal != 28 && startBit != 0)
					{
						MS_ERROR("<H264>");
						MS_ERROR("  [nal:%" PRIu8 "]", kk_nal);

						MS_ERROR("    [subnal:%" PRIu8 ", startBit:%" PRIu8 "]", subnal, startBit);
						MS_ERROR("</H264>");
					}

					break;
				}
			}
			// TODO </TPM>

			uint8_t nal = *data & 0x1F;

			switch (nal)
			{
				// Single NAL unit packet.
				// IDR (instantaneous decoding picture).
				case 7:
				{
					payloadDescriptor->isKeyFrame = true;

					MS_ERROR("------------------------ keyframe: nal = 7");

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
						auto naluSize = Utils::Byte::Get2Bytes(data, offset);

						nal = *(data + offset + sizeof(naluSize)) & 0x1F;

						if (nal == 7)
						{
							payloadDescriptor->isKeyFrame = true;

							MS_ERROR("------------------------ keyframe: nal = 24 (STAP-A)");

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
					nal              = *(data + 1) & 0x1F;
					uint8_t startBit = *(data + 1) & 0x80;

					if (nal == 7 && startBit == 128)
					{
						payloadDescriptor->isKeyFrame = true;

							MS_ERROR("------------------------ keyframe: nal = 27|28 (STAP-A). startBit:%" PRIu8, startBit);
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
			MS_DEBUG_DEV("  isKeyFrame      : %s", this->isKeyFrame ? "true" : "false");
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
