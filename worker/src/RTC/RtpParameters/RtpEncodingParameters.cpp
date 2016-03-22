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

		static const Json::StaticString k_codecPayloadType("codecPayloadType");
		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_fec("fec");
		static const Json::StaticString k_rtx("rtx");

		if (!data.isObject())
			MS_THROW_ERROR("RtpEncodingParameters is not an object");

		// `codecPayloadType` is mandatory.
		if (!data[k_codecPayloadType].isUInt())
			MS_THROW_ERROR("missing `RtpEncodingParameters.codecPayloadType`");

		this->codecPayloadType = (uint8_t)data[k_codecPayloadType].asUInt();

		// `ssrc` is mandatory.
		if (!data[k_ssrc].isUInt())
			MS_THROW_ERROR("missing `RtpEncodingParameters.ssrc`");

		this->ssrc = (uint32_t)data[k_ssrc].asUInt();

		// `fec` is optional.
		if (data[k_fec].isObject())
		{
			this->fec = RtpFecParameters(data[k_fec]);
			this->hasFec = true;
		}

		// `rtx` is optional.
		if (data[k_rtx].isObject())
		{
			this->rtx = RtpRtxParameters(data[k_rtx]);
			this->hasRtx = true;
		}
	}

	RtpEncodingParameters::~RtpEncodingParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpEncodingParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_codecPayloadType("codecPayloadType");
		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_fec("fec");
		static const Json::StaticString k_rtx("rtx");

		Json::Value json(Json::objectValue);

		// Add `codecPayloadType`.
		json[k_codecPayloadType] = (Json::UInt)this->codecPayloadType;

		// Add `ssrc`.
		json[k_ssrc] = (Json::UInt)this->ssrc;

		// Add `fec`
		if (this->hasFec)
			json[k_fec] = this->fec.toJson();

		// Add `rtx`
		if (this->hasRtx)
			json[k_rtx] = this->rtx.toJson();

		return json;
	}
}
