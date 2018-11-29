#define MS_CLASS "RTC::RtpHeaderExtensionUri"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Class variables. */

	// clang-format off
	std::unordered_map<std::string, RtpHeaderExtensionUri::Type> RtpHeaderExtensionUri::string2Type =
	{
		{ "urn:ietf:params:rtp-hdrext:ssrc-audio-level",                RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL  },
		{ "urn:ietf:params:rtp-hdrext:toffset",                         RtpHeaderExtensionUri::Type::TO_OFFSET         },
		{ "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", RtpHeaderExtensionUri::Type::ABS_SEND_TIME     },
		{ "urn:3gpp:video-orientation",                                 RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION },
		{ "urn:ietf:params:rtp-hdrext:sdes:mid",                        RtpHeaderExtensionUri::Type::MID               },
		{ "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id",              RtpHeaderExtensionUri::Type::RTP_STREAM_ID     }
	};
	// clang-format on

	/* Class methods. */

	RtpHeaderExtensionUri::Type RtpHeaderExtensionUri::GetType(std::string& uri)
	{
		MS_TRACE();

		// Force lowcase names.
		Utils::String::ToLowerCase(uri);

		auto it = RtpHeaderExtensionUri::string2Type.find(uri);

		if (it != RtpHeaderExtensionUri::string2Type.end())
			return it->second;

		return RtpHeaderExtensionUri::Type::UNKNOWN;
	}
} // namespace RTC
