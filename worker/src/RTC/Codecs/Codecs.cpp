#define MS_CLASS "RTC::Codecs"
#define MS_LOG_DEV

#include "RTC/Codecs/Codecs.hpp"
#include "Logger.hpp"
#include "RTC/Codecs/VP8.hpp"

namespace RTC
{
	namespace Codecs
	{
		bool IsKnown(const RTC::RtpCodecMimeType& mimeType)
		{
			MS_TRACE();

			if (mimeType.type != RTC::RtpCodecMimeType::Type::VIDEO)
				return false;

			switch (mimeType.subtype)
			{
				case RTC::RtpCodecMimeType::Subtype::VP8:
					return true;
				default:
					return false;
			}
		}

		bool IsKeyFrame(const RTC::RtpCodecMimeType& mimeType, const RTC::RtpPacket* packet)
		{
			MS_TRACE();

			if (!IsKnown(mimeType))
				return false;

			auto data = packet->GetPayload();
			auto len  = packet->GetPayloadLength();

			switch (mimeType.subtype)
			{
				case RTC::RtpCodecMimeType::Subtype::VP8:
					return VP8::IsKeyFrame(data, len);

				default:
					MS_DEBUG_TAG(
					  rtp, "requested key frame for an unsupported codec: %s", mimeType.GetName().c_str());
					return false;
			}
		}
	} // namespace Codecs
} // namespace RTC
