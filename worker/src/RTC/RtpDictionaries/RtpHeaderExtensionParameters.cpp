#define MS_CLASS "RTC::RtpHeaderExtensionParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpHeaderExtensionParameters::RtpHeaderExtensionParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_uri("uri");
		static const Json::StaticString JsonString_id("id");
		static const Json::StaticString JsonString_encrypt("encrypt");
		static const Json::StaticString JsonString_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("RtpHeaderExtensionParameters is not an object");

		// `uri` is mandatory.
		if (!data[JsonString_uri].isString())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.uri");

		this->uri = data[JsonString_uri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty RtpHeaderExtensionParameters.uri");

		// Get the type.
		this->type = RTC::RtpHeaderExtensionUri::GetType(this->uri);

		// `id` is mandatory.
		if (!data[JsonString_id].isUInt())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.id");

		this->id = (uint8_t)data[JsonString_id].asUInt();

		// `encrypt` is optional.
		if (data[JsonString_encrypt].isBool())
			this->encrypt = data[JsonString_encrypt].asBool();

		// `parameters` is optional.
		if (data[JsonString_parameters].isObject())
			this->parameters.Set(data[JsonString_parameters]);
	}

	Json::Value RtpHeaderExtensionParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_uri("uri");
		static const Json::StaticString JsonString_id("id");
		static const Json::StaticString JsonString_encrypt("encrypt");
		static const Json::StaticString JsonString_parameters("parameters");

		Json::Value json(Json::objectValue);

		// Add `uri`.
		json[JsonString_uri] = this->uri;

		// Add `id`.
		json[JsonString_id] = (Json::UInt)this->id;

		// Add `encrypt`.
		json[JsonString_encrypt] = this->encrypt;

		// Add `parameters`.
		json[JsonString_parameters] = this->parameters.toJson();

		return json;
	}
} // namespace RTC
