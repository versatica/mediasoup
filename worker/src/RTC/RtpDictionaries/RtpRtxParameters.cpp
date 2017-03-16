#define MS_CLASS "RTC::RtpRtxParameters"
// #define MS_LOG_DEV

#include "RTC/RtpDictionaries.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpRtxParameters::RtpRtxParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("RtpRtxParameters is not an object");

		// `ssrc` is optional.
		if (data[k_ssrc].isUInt())
			this->ssrc = (uint32_t)data[k_ssrc].asUInt();
	}

	Json::Value RtpRtxParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `ssrc`.
		if (this->ssrc)
			json[k_ssrc] = (Json::UInt)this->ssrc;

		return json;
	}
}
