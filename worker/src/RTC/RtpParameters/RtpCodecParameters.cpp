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
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("`RtpCodecParameters` is not an object");

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing `RtpCodecParameters.name`");

		this->name = data[k_name].asString();
		if (this->name.empty())
			MS_THROW_ERROR("empty `RtpCodecParameters.name`");

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing `RtpCodecParameters.payloadType`");

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
		{
			auto& json_parameters = data[k_parameters];

			for (Json::Value::iterator it = json_parameters.begin(); it != json_parameters.end(); ++it)
			{
				std::string key = it.key().asString();
				Json::Value value = (*it);

				switch (value.type())
				{
					case Json::booleanValue:
					{
						bool booleanValue = value.asBool();

						this->parameters[key] = RTC::CustomParameterValue(booleanValue);

						break;
					}

					case Json::uintValue:
					case Json::intValue:
					{
						uint32_t integerValue = (uint32_t)value.asUInt();

						this->parameters[key] = RTC::CustomParameterValue(integerValue);

						break;
					}

					case Json::realValue:
					{
						double doubleValue = value.asDouble();

						this->parameters[key] = RTC::CustomParameterValue(doubleValue);

						break;
					}

					case Json::stringValue:
					{
						std::string stringValue = value.asString();

						this->parameters[key] = RTC::CustomParameterValue(stringValue);

						break;
					}

					default:
						;  // Just ignore other value types
				}
			}
		}
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

		// Add `rtcpFeedback` (if any).
		if (!this->rtcpFeedback.empty())
		{
			json[k_rtcpFeedback] = Json::arrayValue;

			for (auto& entry : this->rtcpFeedback)
			{
				json[k_rtcpFeedback].append(entry.toJson());
			}
		}

		// Add `parameters` (if any).
		if (this->parameters.size() > 0)
		{
			Json::Value json_parameters(Json::objectValue);

			for (auto& kv : this->parameters)
			{
				const std::string& key = kv.first;
				RTC::CustomParameterValue& parameterValue = kv.second;

				switch (parameterValue.type)
				{
					case RTC::CustomParameterValue::Type::BOOLEAN:
						json_parameters[key] = parameterValue.booleanValue;
						break;

					case RTC::CustomParameterValue::Type::INTEGER:
						json_parameters[key] = (Json::UInt)parameterValue.integerValue;
						break;

					case RTC::CustomParameterValue::Type::DOUBLE:
						json_parameters[key] = parameterValue.doubleValue;
						break;

					case RTC::CustomParameterValue::Type::STRING:
						json_parameters[key] = parameterValue.stringValue;
						break;
				}
			}

			json[k_parameters] = json_parameters;
		}

		return json;
	}
}
