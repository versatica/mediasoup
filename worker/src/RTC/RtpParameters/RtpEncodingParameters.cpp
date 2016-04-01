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
		static const Json::StaticString k_rtxSsrc("rtxSsrc");
		static const Json::StaticString k_fecSsrc("fecSsrc");
		static const Json::StaticString k_codecPayloadType("codecPayloadType");

		if (!data.isObject())
			MS_THROW_ERROR("`RtpEncodingParameters is not an object");

		// `ssrc` is mandatory.
		if (!data[k_ssrc].isUInt())
			MS_THROW_ERROR("missing `RtpEncodingParameters.ssrc`");

		this->ssrc = (uint32_t)data[k_ssrc].asUInt();

		// `rtxSsrc` is optional.
		if (data[k_rtxSsrc].isUInt())
			this->rtxSsrc = (uint32_t)data[k_rtxSsrc].asUInt();

		// `fecSsrc` is optional.
		if (data[k_fecSsrc].isUInt())
			this->fecSsrc = (uint32_t)data[k_fecSsrc].asUInt();

		// `codecPayloadType` is optional.
		if (data[k_codecPayloadType].isUInt())
			this->codecPayloadType = (uint8_t)data[k_codecPayloadType].asUInt();
	}

	RtpEncodingParameters::~RtpEncodingParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpEncodingParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_rtxSsrc("rtxSsrc");
		static const Json::StaticString k_fecSsrc("fecSsrc");
		static const Json::StaticString k_codecPayloadType("codecPayloadType");

		Json::Value json(Json::objectValue);

		// Add `ssrc`.
		json[k_ssrc] = (Json::UInt)this->ssrc;

		// Add `rtxSsrc`.
		if (this->rtxSsrc)
			json[k_rtxSsrc] = (Json::UInt)this->rtxSsrc;

		// Add `fecSsrc`.
		if (this->fecSsrc)
			json[k_fecSsrc] = (Json::UInt)this->fecSsrc;

		// Add `codecPayloadType`.
		if (this->codecPayloadType)
			json[k_codecPayloadType] = (Json::UInt)this->codecPayloadType;

		return json;
	}
}
