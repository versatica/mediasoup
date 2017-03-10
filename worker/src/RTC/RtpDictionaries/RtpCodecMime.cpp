#define MS_CLASS "RTC::RtpCodecMime"
// #define MS_LOG_DEV

#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Class variables. */

	std::unordered_map<std::string, RtpCodecMime::Type> RtpCodecMime::string2Type =
	{
		{ "audio", RtpCodecMime::Type::AUDIO },
		{ "video", RtpCodecMime::Type::VIDEO }
	};

	std::map<RtpCodecMime::Type, std::string> RtpCodecMime::type2String =
	{
		{ RtpCodecMime::Type::AUDIO, "audio" },
		{ RtpCodecMime::Type::VIDEO, "video" }
	};

	std::unordered_map<std::string, RtpCodecMime::Subtype> RtpCodecMime::string2Subtype =
	{
		// Audio codecs:
		{ "opus",            RtpCodecMime::Subtype::OPUS            },
		{ "pcma",            RtpCodecMime::Subtype::PCMA            },
		{ "pcmu",            RtpCodecMime::Subtype::PCMU            },
		{ "isac",            RtpCodecMime::Subtype::ISAC            },
		{ "g722",            RtpCodecMime::Subtype::G722            },
		{ "ilbc",            RtpCodecMime::Subtype::ILBC            },
		// Video codecs:
		{ "vp8",             RtpCodecMime::Subtype::VP8             },
		{ "vp9",             RtpCodecMime::Subtype::VP9             },
		{ "h264",            RtpCodecMime::Subtype::H264            },
		{ "h265",            RtpCodecMime::Subtype::H265            },
		// Complementary codecs:
		{ "cn",              RtpCodecMime::Subtype::CN              },
		{ "telephone-event", RtpCodecMime::Subtype::TELEPHONE_EVENT },
		// Feature codecs:
		{ "rtx",             RtpCodecMime::Subtype::RTX             },
		{ "ulpfec",          RtpCodecMime::Subtype::ULPFEC          },
		{ "flexfec",         RtpCodecMime::Subtype::FLEXFEC         },
		{ "red",             RtpCodecMime::Subtype::RED             }
	};

	std::map<RtpCodecMime::Subtype, std::string> RtpCodecMime::subtype2String =
	{
		// Audio codecs:
		{ RtpCodecMime::Subtype::OPUS,            "opus"            },
		{ RtpCodecMime::Subtype::PCMA,            "PCMA"            },
		{ RtpCodecMime::Subtype::PCMU,            "PCMU"            },
		{ RtpCodecMime::Subtype::ISAC,            "ISAC"            },
		{ RtpCodecMime::Subtype::G722,            "G722"            },
		{ RtpCodecMime::Subtype::ILBC,            "iLBC"            },
		// Video codecs:
		{ RtpCodecMime::Subtype::VP8,             "VP8"             },
		{ RtpCodecMime::Subtype::VP9,             "VP9"             },
		{ RtpCodecMime::Subtype::H264,            "H264"            },
		{ RtpCodecMime::Subtype::H265,            "H265"            },
		// Complementary codecs:
		{ RtpCodecMime::Subtype::CN,              "CN"              },
		{ RtpCodecMime::Subtype::TELEPHONE_EVENT, "telephone-event" },
		// Feature codecs:
		{ RtpCodecMime::Subtype::RTX,             "rtx"             },
		{ RtpCodecMime::Subtype::ULPFEC,          "ulpfec"          },
		{ RtpCodecMime::Subtype::FLEXFEC,         "flexfec"         },
		{ RtpCodecMime::Subtype::RED,             "red"             }
	};

	/* Instance methods. */

	void RtpCodecMime::SetName(std::string& name)
	{
		MS_TRACE();

		auto slash_pos = name.find('/');

		if (slash_pos == std::string::npos || slash_pos == 0 || slash_pos == name.length() - 1)
			MS_THROW_ERROR("wrong codec MIME");

		std::string type = name.substr(0, slash_pos);
		std::string subtype = name.substr(slash_pos + 1);

		// Force lowcase names.
		Utils::String::ToLowerCase(type);
		Utils::String::ToLowerCase(subtype);

		// Set MIME type.
		{
			auto it = RtpCodecMime::string2Type.find(type);

			if (it != RtpCodecMime::string2Type.end())
				this->type = it->second;
			else
				MS_THROW_ERROR("unknown codec MIME type");
		}

		// Set MIME subtype.
		{
			auto it = RtpCodecMime::string2Subtype.find(subtype);

			if (it != RtpCodecMime::string2Subtype.end())
				this->subtype = it->second;
			else
				MS_THROW_ERROR("unknown codec MIME subtype");
		}

		// Set name.
		this->name = type2String[this->type] + "/" + subtype2String[this->subtype];
	}
}
