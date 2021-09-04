#define MS_CLASS "RTC::Media"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, Media::Kind> Media::string2Kind =
	{
		{ "",      Media::Kind::ALL   },
		{ "audio", Media::Kind::AUDIO },
		{ "video", Media::Kind::VIDEO }
	};
	absl::flat_hash_map<Media::Kind, std::string> Media::kind2String =
	{
		{ Media::Kind::ALL,   ""      },
		{ Media::Kind::AUDIO, "audio" },
		{ Media::Kind::VIDEO, "video" }
	};
	// clang-format on

	/* Class methods. */

	Media::Kind Media::GetKind(std::string& str)
	{
		MS_TRACE();

		// Force lowcase kind.
		Utils::String::ToLowerCase(str);

		auto it = Media::string2Kind.find(str);

		if (it == Media::string2Kind.end())
			MS_THROW_TYPE_ERROR("invalid media kind [kind:%s]", str.c_str());

		return it->second;
	}

	Media::Kind Media::GetKind(std::string&& str)
	{
		MS_TRACE();

		// Force lowcase kind.
		Utils::String::ToLowerCase(str);

		auto it = Media::string2Kind.find(str);

		if (it == Media::string2Kind.end())
			MS_THROW_TYPE_ERROR("invalid media kind [kind:%s]", str.c_str());

		return it->second;
	}

	const std::string& Media::GetString(Media::Kind kind)
	{
		MS_TRACE();

		return Media::kind2String.at(kind);
	}
} // namespace RTC
