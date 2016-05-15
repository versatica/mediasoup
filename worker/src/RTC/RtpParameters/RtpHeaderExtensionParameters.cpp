#define MS_CLASS "RTC::RtpHeaderExtensionParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpHeaderExtensionParameters::RtpHeaderExtensionParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_uri("uri");
		static const Json::StaticString k_id("id");
		static const Json::StaticString k_encrypt("encrypt");
		static const Json::StaticString k_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("`RtpHeaderExtensionParameters` is not an object");

		// `uri` is mandatory.
		if (!data[k_uri].isString())
			MS_THROW_ERROR("missing `RtpHeaderExtensionParameters.uri`");

		this->uri = data[k_uri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty `RtpHeaderExtensionParameters.uri`");

		// `id` is mandatory.
		if (!data[k_id].asUInt())
			MS_THROW_ERROR("missing `RtpHeaderExtensionParameters.id`");

		this->id = (uint16_t)data[k_id].asUInt();

		// `encrypt` is optional.
		if (data[k_encrypt].isBool())
			this->encrypt = data[k_encrypt].asBool();

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

	RtpHeaderExtensionParameters::~RtpHeaderExtensionParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpHeaderExtensionParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_uri("uri");
		static const Json::StaticString k_id("id");
		static const Json::StaticString k_encrypt("encrypt");
		static const Json::StaticString k_parameters("parameters");

		Json::Value json(Json::objectValue);

		// Add `uri`.
		json[k_uri] = this->uri;

		// Add `id`.
		json[k_id] = (Json::UInt)this->id;

		// Add `encrypt`.
		json[k_encrypt] = this->encrypt;

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
