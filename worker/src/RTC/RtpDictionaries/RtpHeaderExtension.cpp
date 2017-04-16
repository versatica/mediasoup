#define MS_CLASS "RTC::RtpHeaderExtension"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpHeaderExtension::RtpHeaderExtension(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_kind("kind");
		static const Json::StaticString JsonString_uri("uri");
		static const Json::StaticString JsonString_preferredId("preferredId");
		static const Json::StaticString JsonString_preferredEncrypt("preferredEncrypt");

		if (!data.isObject())
			MS_THROW_ERROR("RtpHeaderExtension is not an object");

		// `kind` is mandatory.
		if (!data[JsonString_kind].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.kind");

		std::string kind = data[JsonString_kind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// `uri` is mandatory.
		if (!data[JsonString_uri].isString())
			MS_THROW_ERROR("missing RtpHeaderExtension.uri");

		this->uri = data[JsonString_uri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty RtpHeaderExtension.uri");

		// Get the type.
		this->type = RTC::RtpHeaderExtensionUri::GetType(this->uri);

		// `preferredId` is mandatory.
		if (!data[JsonString_preferredId].isUInt())
			MS_THROW_ERROR("missing RtpHeaderExtension.preferredId");

		this->preferredId = (uint8_t)data[JsonString_preferredId].asUInt();

		// `preferredEncrypt` is optional.
		if (data[JsonString_preferredEncrypt].isBool())
			this->preferredEncrypt = data[JsonString_preferredEncrypt].asBool();
	}

	Json::Value RtpHeaderExtension::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_kind("kind");
		static const Json::StaticString JsonString_uri("uri");
		static const Json::StaticString JsonString_preferredId("preferredId");
		static const Json::StaticString JsonString_preferredEncrypt("preferredEncrypt");

		Json::Value json(Json::objectValue);

		// Add `kind`.
		json[JsonString_kind] = RTC::Media::GetJsonString(this->kind);

		// Add `uri`.
		json[JsonString_uri] = this->uri;

		// Add `preferredId`.
		json[JsonString_preferredId] = (Json::UInt)this->preferredId;

		// Add `preferredEncrypt`.
		json[JsonString_preferredEncrypt] = this->preferredEncrypt;

		return json;
	}
} // namespace RTC
