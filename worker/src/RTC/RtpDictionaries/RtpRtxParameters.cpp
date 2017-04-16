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

		static const Json::StaticString JsonString_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("RtpRtxParameters is not an object");

		// `ssrc` is optional.
		if (data[JsonString_ssrc].isUInt())
			this->ssrc = (uint32_t)data[JsonString_ssrc].asUInt();
	}

	Json::Value RtpRtxParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `ssrc`.
		if (this->ssrc)
			json[JsonString_ssrc] = (Json::UInt)this->ssrc;

		return json;
	}
} // namespace RTC
