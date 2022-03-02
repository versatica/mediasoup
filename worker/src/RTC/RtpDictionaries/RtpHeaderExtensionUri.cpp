#define MS_CLASS "RTC::RtpHeaderExtensionUri"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, RtpHeaderExtensionUri::Type> RtpHeaderExtensionUri::string2Type =
	{
		{ "urn:ietf:params:rtp-hdrext:sdes:mid",                                       RtpHeaderExtensionUri::Type::MID                    },
		{ "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id",                             RtpHeaderExtensionUri::Type::RTP_STREAM_ID          },
		{ "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id",                    RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID },
		{ "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",                RtpHeaderExtensionUri::Type::ABS_SEND_TIME          },
		{ "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", RtpHeaderExtensionUri::Type::TRANSPORT_WIDE_CC_01   },
		// NOTE: Remove this once framemarking draft becomes RFC.
		{ "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07",              RtpHeaderExtensionUri::Type::FRAME_MARKING_07       },
		{ "urn:ietf:params:rtp-hdrext:framemarking",                                   RtpHeaderExtensionUri::Type::FRAME_MARKING          },
		{ "urn:ietf:params:rtp-hdrext:ssrc-audio-level",                               RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL       },
		{ "urn:3gpp:video-orientation",                                                RtpHeaderExtensionUri::Type::VIDEO_ORIENTATION      },
		{ "urn:ietf:params:rtp-hdrext:toffset",                                        RtpHeaderExtensionUri::Type::TOFFSET                },
		{ "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time",             RtpHeaderExtensionUri::Type::ABS_CAPTURE_TIME       },
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
