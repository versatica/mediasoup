#ifndef MS_RTC_CODECS_HPP
#define MS_RTC_CODECS_HPP

#include "common.hpp"
#include "RTC/Codecs/H264.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/Codecs/VP8.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		bool CanBeKeyFrame(const RTC::RtpCodecMimeType& mimeType);
		void ProcessRtpPacket(RTC::RtpPacket* packet, const RTC::RtpCodecMimeType& mimeType);
		EncodingContext* GetEncodingContext(const RTC::RtpCodecMimeType& mimeType);

		/* Inline namespace methods. */

		inline bool CanBeKeyFrame(const RTC::RtpCodecMimeType& mimeType)
		{
			if (mimeType.type != RTC::RtpCodecMimeType::Type::VIDEO)
				return false;

			switch (mimeType.subtype)
			{
				case RTC::RtpCodecMimeType::Subtype::VP8:
				case RTC::RtpCodecMimeType::Subtype::H264:
					return true;
				default:
					return false;
			}
		}

		inline EncodingContext* GetEncodingContext(const RTC::RtpCodecMimeType& mimeType)
		{
			switch (mimeType.type)
			{
				case RTC::RtpCodecMimeType::Type::VIDEO:
				{
					switch (mimeType.subtype)
					{
						case RTC::RtpCodecMimeType::Subtype::VP8:
							return new RTC::Codecs::VP8::EncodingContext();
						case RTC::RtpCodecMimeType::Subtype::H264:
							return new RTC::Codecs::H264::EncodingContext();
						default:
							return nullptr;
					}
				}

				default:
				{
					return nullptr;
				}
			}
		}
	} // namespace Codecs
} // namespace RTC

#endif
