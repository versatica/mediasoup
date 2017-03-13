#define MS_CLASS "RTC::RtpHeaderExtensionParameters"
// #define MS_LOG_DEV

#include "RTC/RtpDictionaries.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"

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

		// Get the type.
		this->type = RTC::RtpHeaderExtensionUri::GetType(this->uri);

		// `id` is mandatory.
		if (!data[k_id].isUInt())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.id");

		this->id = (uint16_t)data[k_id].asUInt();

		// `encrypt` is optional.
		if (data[k_encrypt].isBool())
			this->encrypt = data[k_encrypt].asBool();

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			this->parameters.Set(data[k_parameters]);
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

		// Add `parameters`.
		json[k_parameters] = this->parameters.toJson();

		return json;
	}
}
