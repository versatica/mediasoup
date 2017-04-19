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

		static const Json::StaticString JsonStringMechanism{ "mechanism" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpFecParameters is not an object");

		// `mechanism` is mandatory.
		if (!data[JsonStringMechanism].isString())
			MS_THROW_ERROR("missing RtpFecParameters.mechanism");

		this->mechanism = data[JsonStringMechanism].asString();

		// `ssrc` is optional.
		if (data[JsonStringSsrc].isUInt())
			this->ssrc = uint32_t{ data[JsonStringSsrc].asUInt() };
	}

	Json::Value RtpFecParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringMechanism{ "mechanism" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		Json::Value json(Json::objectValue);

		// Add `mechanism`.
		json[JsonStringMechanism] = this->mechanism;

		// Add `ssrc`.
		if (this->ssrc != 0u)
			json[JsonStringSsrc] = Json::UInt{ this->ssrc };

		return json;
	}
} // namespace RTC
