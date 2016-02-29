#define MS_CLASS "RTC::RtpCodecParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");

		if (!data.isObject())
			MS_THROW_ERROR("data is not an object");

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing `codec.name`");

		this->name = data[k_name].asString();
		if (this->name.empty())
			MS_THROW_ERROR("empty `codec.name`");

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing `codec.payloadType`");

		this->payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `clockRate` is optional.
		if (data[k_clockRate].isUInt())
			this->clockRate = (uint32_t)data[k_clockRate].asUInt();
	}

	RtpCodecParameters::~RtpCodecParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString v_both("");
		static const Json::StaticString v_audio("audio");
		static const Json::StaticString v_video("video");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");

		Json::Value json(Json::objectValue);

		// Add `name`.
		json[k_name] = this->name;

		// Add `payloadType`.
		json[k_payloadType] = (Json::UInt)this->payloadType;

		// Add `clockRate`.
		if (this->clockRate)
			json[k_clockRate] = (Json::UInt)this->clockRate;
		else
			json[k_clockRate] = Json::nullValue;

		return json;
	}
}
