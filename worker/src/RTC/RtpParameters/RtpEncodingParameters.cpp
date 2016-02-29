#define MS_CLASS "RTC::RtpEncodingParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpEncodingParameters::RtpEncodingParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("RtpEncodingParameters is not an object");

		// `ssrc` is mandatory.
		if (!data[k_ssrc].isUInt())
			MS_THROW_ERROR("missing `RtpEncodingParameters.ssrc`");

		this->ssrc = (uint32_t)data[k_ssrc].asUInt();
	}

	RtpEncodingParameters::~RtpEncodingParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpEncodingParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `ssrc`.
		json[k_ssrc] = this->ssrc;

		return json;
	}
}
