#define MS_CLASS "RTC::FecCodecParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	FecCodecParameters::FecCodecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_mechanism("mechanism");
		static const Json::StaticString k_payloadType("payloadType");

		if (!data.isObject())
			MS_THROW_ERROR("`FecCodecParameters` is not an object");

		// `mechanism` is mandatory.
		if (!data[k_mechanism].isString())
			MS_THROW_ERROR("missing `FecCodecParameters.mechanism`");

		this->mechanism = data[k_mechanism].asString();

		// `payloadType` is optional.
		if (data[k_payloadType].isUInt())
			this->payloadType = (uint8_t)data[k_payloadType].asUInt();
	}

	FecCodecParameters::~FecCodecParameters()
	{
		MS_TRACE();
	}

	Json::Value FecCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_mechanism("mechanism");
		static const Json::StaticString k_payloadType("payloadType");

		Json::Value json(Json::objectValue);

		// Add `mechanism`.
		json[k_mechanism] = this->mechanism;

		// Add `payloadType`.
		if (this->payloadType)
			json[k_payloadType] = (Json::UInt)this->payloadType;

		return json;
	}
}
