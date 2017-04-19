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

		static const Json::StaticString JsonStringUri{ "uri" };
		static const Json::StaticString JsonStringId{ "id" };
		static const Json::StaticString JsonStringEncrypt{ "encrypt" };
		static const Json::StaticString JsonStringParameters{ "parameters" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpHeaderExtensionParameters is not an object");

		// `uri` is mandatory.
		if (!data[JsonStringUri].isString())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.uri");

		this->uri = data[JsonStringUri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty RtpHeaderExtensionParameters.uri");

		// Get the type.
		this->type = RTC::RtpHeaderExtensionUri::GetType(this->uri);

		// `id` is mandatory.
		if (!data[JsonStringId].isUInt())
			MS_THROW_ERROR("missing RtpHeaderExtensionParameters.id");

		this->id = static_cast<uint8_t>(data[JsonStringId].asUInt());

		// `encrypt` is optional.
		if (data[JsonStringEncrypt].isBool())
			this->encrypt = data[JsonStringEncrypt].asBool();

		// `parameters` is optional.
		if (data[JsonStringParameters].isObject())
			this->parameters.Set(data[JsonStringParameters]);
	}

	Json::Value RtpHeaderExtensionParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringUri{ "uri" };
		static const Json::StaticString JsonStringId{ "id" };
		static const Json::StaticString JsonStringEncrypt{ "encrypt" };
		static const Json::StaticString JsonStringParameters{ "parameters" };

		Json::Value json(Json::objectValue);

		// Add `uri`.
		json[JsonStringUri] = this->uri;

		// Add `id`.
		json[JsonStringId] = Json::UInt{ this->id };

		// Add `encrypt`.
		json[JsonStringEncrypt] = this->encrypt;

		// Add `parameters`.
		json[JsonStringParameters] = this->parameters.ToJson();

		return json;
	}
} // namespace RTC
