#define MS_CLASS "RTC::RtxCodecParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtxCodecParameters::RtxCodecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtxTime("rtxTime");

		if (!data.isObject())
			MS_THROW_ERROR("`RtxCodecParameters is not an object");

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing `RtpEncodingParameters.payloadType`");

		this->payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `rtxTime` is optional.
		if (data[k_rtxTime].isUInt())
			this->rtxTime = (uint32_t)data[k_rtxTime].asUInt();
	}

	RtxCodecParameters::~RtxCodecParameters()
	{
		MS_TRACE();
	}

	Json::Value RtxCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtxTime("rtxTime");

		Json::Value json(Json::objectValue);

		// Add `payloadType`.
		json[k_payloadType] = (Json::UInt)this->payloadType;

		// Add `rtxTime`.
		if (this->rtxTime)
			json[k_rtxTime] = (Json::UInt)this->rtxTime;

		return json;
	}
}
