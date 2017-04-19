#define MS_CLASS "RTC::RtpRtxParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpRtxParameters::RtpRtxParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpRtxParameters is not an object");

		// `ssrc` is optional.
		if (data[JsonStringSsrc].isUInt())
			this->ssrc = uint32_t{ data[JsonStringSsrc].asUInt() };
	}

	Json::Value RtpRtxParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		Json::Value json(Json::objectValue);

		// Add `ssrc`.
		if (this->ssrc != 0u)
			json[JsonStringSsrc] = Json::UInt{ this->ssrc };

		return json;
	}
} // namespace RTC
