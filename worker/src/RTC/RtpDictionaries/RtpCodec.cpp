#define MS_CLASS "RTC::RtpCodec"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodec::RtpCodec(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodec is not an object");

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing RtpCodec.name");

		std::string name = data[k_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		// `clockRate` is mandatory.
		if (!data[k_clockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodec.clockRate");

		this->clockRate = (uint32_t)data[k_clockRate].asUInt();

		// `maxptime` is optional.
		if (data[k_maxptime].isUInt())
			this->maxptime = (uint32_t)data[k_maxptime].asUInt();

		// `ptime` is optional.
		if (data[k_ptime].isUInt())
			this->ptime = (uint32_t)data[k_ptime].asUInt();

		// `numChannels` is optional.
		if (data[k_numChannels].isUInt())
			this->numChannels = (uint32_t)data[k_numChannels].asUInt();

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			this->parameters.Set(data[k_parameters]);
	}

	void RtpCodec::toJson(Json::Value& json)
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_parameters("parameters");

		// Add `name`.
		json[k_name] = this->mime.GetName();

		// Add `clockRate`.
		json[k_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[k_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[k_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels > 1)
			json[k_numChannels] = (Json::UInt)this->numChannels;

		// Add `parameters`.
		json[k_parameters] = this->parameters.toJson();
	}
}
