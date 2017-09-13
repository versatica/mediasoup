#define MS_CLASS "RTC::Codecs::VP8"
#define MS_LOG_DEV

#include "RTC/Codecs/VP8.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		bool VP8::IsKeyFrame(const uint8_t* data, size_t len)
		{
			if (len < 1)
				return false;

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
				{
					if (len < offset + 1)
						return false;
				}

				byte = data[offset++];

				i = (byte >> 7) & 0x1;
				l = (byte >> 6) & 0x1;
				t = (byte >> 5) & 0x1;
				k = (byte >> 4) & 0x1;
			}

			if (i)
			{
				if (len < offset++)
				{
					return false;
				}

				if ((byte >> 7) & 0x1)
				{
					if (len < offset++)
						return false;
				}
			}

			if (l)
			{
				if (len < offset++)
				{
					return false;
				}
			}

			if (t || k)
			{
				if (len < offset++)
				{
					return false;
				}
			}

			return start && pid == 0 && (!(data[offset] & 0x1));
		}
	} // namespace Codecs
} // namespace RTC
