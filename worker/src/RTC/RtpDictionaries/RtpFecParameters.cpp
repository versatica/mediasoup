#define MS_CLASS "RTC::RtpFecParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpFecParameters::RtpFecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_mechanism("mechanism");
		static const Json::StaticString JsonString_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("RtpFecParameters is not an object");

		// `mechanism` is mandatory.
		if (!data[JsonString_mechanism].isString())
			MS_THROW_ERROR("missing RtpFecParameters.mechanism");

		this->mechanism = data[JsonString_mechanism].asString();

		// `ssrc` is optional.
		if (data[JsonString_ssrc].isUInt())
			this->ssrc = (uint32_t)data[JsonString_ssrc].asUInt();
	}

	Json::Value RtpFecParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_mechanism("mechanism");
		static const Json::StaticString JsonString_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `mechanism`.
		json[JsonString_mechanism] = this->mechanism;

		// Add `ssrc`.
		if (this->ssrc)
			json[JsonString_ssrc] = (Json::UInt)this->ssrc;

		return json;
	}
} // namespace RTC
