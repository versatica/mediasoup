#define MS_CLASS "RTC::RtpHeaderExtensionParameters"

#include "RTC/RtpDictionaries.h"
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
			MS_THROW_ERROR("RtpHeaderExtensionParameters is not an object");

		// `uri` is mandatory.
		if (!data[k_uri].isString())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.uri");

		this->uri = data[k_uri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty RtpHeaderExtensionParameters.uri");

		// `id` is mandatory.
		if (!data[k_id].asUInt())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.id");

		this->id = (uint16_t)data[k_id].asUInt();

		// `encrypt` is optional.
		if (data[k_encrypt].isBool())
			this->encrypt = data[k_encrypt].asBool();

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			RTC::FillCustomParameters(this->parameters, data[k_parameters]);
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
				auto& parameterValue = kv.second;

				json_parameters[key] = parameterValue.toJson();
			}

			json[k_parameters] = json_parameters;
		}

		return json;
	}
}
