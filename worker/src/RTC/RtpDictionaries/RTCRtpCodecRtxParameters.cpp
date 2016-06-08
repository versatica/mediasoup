#define MS_CLASS "RTC::RTCRtpCodecRtxParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RTCRtpCodecRtxParameters::RTCRtpCodecRtxParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtxTime("rtxTime");

		if (!data.isObject())
			MS_THROW_ERROR("RTCRtpCodecRtxParameters is not an object");

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing RTCRtpCodecRtxParameters.payloadType");

		this->payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `rtxTime` is optional.
		if (data[k_rtxTime].isUInt())
			this->rtxTime = (uint32_t)data[k_rtxTime].asUInt();
	}

	RTCRtpCodecRtxParameters::~RTCRtpCodecRtxParameters()
	{
		MS_TRACE();
	}

	Json::Value RTCRtpCodecRtxParameters::toJson()
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
