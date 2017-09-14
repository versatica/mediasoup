#define MS_CLASS "RTC::Codecs::VP8"
// #define MS_LOG_DEV

#include "RTC/Codecs/VP8.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		void VP8::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto data = packet->GetPayload();
			auto len  = packet->GetPayloadLength();

			if (len < 1)
				return;

			size_t offset = 0;
			uint8_t byte  = data[offset++];

			uint8_t x     = (byte >> 7) & 0x1;
			uint8_t start = (byte >> 4) & 0x1;
			uint8_t pid   = byte & 0x07;

			uint8_t i = 0;
			uint8_t l = 0;
			uint8_t t = 0;
			uint8_t k = 0;

			if (x)
			{
				if (len < offset + 1)
					return;

				byte = data[offset++];

				i = (byte >> 7) & 0x1;
				l = (byte >> 6) & 0x1;
				t = (byte >> 5) & 0x1;
				k = (byte >> 4) & 0x1;
			}

			if (i)
			{
				if (len < offset++)
					return;

				if ((byte >> 7) & 0x1)
				{
					if (len < offset++)
						return;
				}
			}

			if (l)
			{
				if (len < offset++)
					return;
			}

			if (t || k)
			{
				if (len < offset++)
					return;
			}

			if (start && pid == 0 && (!(data[offset] & 0x1)))
				packet->SetKeyFrame(true);
		}
	} // namespace Codecs
} // namespace RTC
