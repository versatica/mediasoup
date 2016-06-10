#define MS_CLASS "RTC::RtpFecParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpFecParameters::RtpFecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_mechanism("mechanism");
		static const Json::StaticString k_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("RtpFecParameters is not an object");

		// `mechanism` is mandatory.
		if (!data[k_mechanism].isString())
			MS_THROW_ERROR("missing RtpFecParameters.mechanism");

		this->mechanism = data[k_mechanism].asString();

		// `ssrc` is optional.
		if (data[k_ssrc].isUInt())
			this->ssrc = (uint32_t)data[k_ssrc].asUInt();
	}

	Json::Value RtpFecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_mechanism("mechanism");
		static const Json::StaticString k_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `mechanism`.
		json[k_mechanism] = this->mechanism;

		// Add `ssrc`.
		if (this->ssrc)
			json[k_ssrc] = (Json::UInt)this->ssrc;

		return json;
	}
}
