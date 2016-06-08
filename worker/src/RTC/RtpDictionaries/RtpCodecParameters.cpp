#define MS_CLASS "RTC::RtpCodecParameters"

#include "RTC/RtpDictionaries.h"
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
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecParameters is not an object");

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing RtpCodecParameters.name");

		std::string name = data[k_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		// Override/normalize given `name`.
		this->name = this->mime.ToString();

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.payloadType");

		this->payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `clockRate` is optional.
		if (data[k_clockRate].isUInt())
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

		// `rtx` is optional.
		if (data[k_rtx].isObject())
		{
			this->rtx = RTCRtpCodecRtxParameters(data[k_rtx]);
			this->hasRtx = true;
		}

		// `rtcpFeedback` is optional.
		if (data[k_rtcpFeedback].isArray())
		{
			auto& json_rtcpFeedback = data[k_rtcpFeedback];

			for (Json::UInt i = 0; i < json_rtcpFeedback.size(); i++)
			{
				RtcpFeedback rtcpFeedback(json_rtcpFeedback[i]);

				// Append to the rtcpFeedback vector.
				this->rtcpFeedback.push_back(rtcpFeedback);
			}
		}

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			RTC::FillCustomParameters(this->parameters, data[k_parameters]);
	}

	RtpCodecParameters::~RtpCodecParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");

		Json::Value json(Json::objectValue);

		// Add `name`.
		json[k_name] = this->name;

		// Add `payloadType`.
		json[k_payloadType] = (Json::UInt)this->payloadType;

		// Add `clockRate`.
		if (this->clockRate)
			json[k_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[k_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[k_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels)
			json[k_numChannels] = (Json::UInt)this->numChannels;

		// Add `rtx`
		if (this->hasRtx)
			json[k_rtx] = this->rtx.toJson();

		// Add `rtcpFeedback`.
		json[k_rtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[k_rtcpFeedback].append(entry.toJson());
		}

		// Add `parameters`.
		Json::Value json_parameters(Json::objectValue);

		for (auto& kv : this->parameters)
		{
			const std::string& key = kv.first;
			auto& parameterValue = kv.second;

			json_parameters[key] = parameterValue.toJson();
		}

		json[k_parameters] = json_parameters;

		return json;
	}
}
