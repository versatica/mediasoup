#define MS_CLASS "RTC::Codecs"
// #define MS_LOG_DEV

#include "RTC/Codecs/Codecs.hpp"
#include "Logger.hpp"
#include "RTC/Codecs/VP8.hpp"

namespace RTC
{
	namespace Codecs
	{
		void ProcessRtpPacket(RTC::RtpPacket* packet, const RTC::RtpCodecMimeType& mimeType)
		{
			MS_TRACE();

			switch (mimeType.subtype)
			{
				case RTC::RtpCodecMimeType::Subtype::VP8:
				{
					VP8::ProcessRtpPacket(packet);

					break;
				}

				default:
					;
			}
		}
	} // namespace Codecs
} // namespace RTC
