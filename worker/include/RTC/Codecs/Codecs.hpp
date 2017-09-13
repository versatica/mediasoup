#ifndef MS_RTC_CODECS_HPP
#define MS_RTC_CODECS_HPP

#include "common.hpp"
#include "RTC/Codecs/VP8.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		bool IsKnown(const RTC::RtpCodecMimeType& mimeType);
		bool IsKeyFrame(const RTC::RtpCodecMimeType& mimeType, const RTC::RtpPacket* packet);
	} // namespace Codecs
} // namespace RTC

#endif
