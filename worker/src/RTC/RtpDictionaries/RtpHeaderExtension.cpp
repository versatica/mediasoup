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

		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringUri{ "uri" };
		static const Json::StaticString JsonStringPreferredId{ "preferredId" };
		static const Json::StaticString JsonStringPreferredEncrypt{ "preferredEncrypt" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpHeaderExtension is not an object");

		// `kind` is mandatory.
		if (!data[JsonStringKind].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.kind");

		std::string kind = data[JsonStringKind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// `uri` is mandatory.
		if (!data[JsonStringUri].isString())
			MS_THROW_ERROR("missing RtpHeaderExtension.uri");

		this->uri = data[JsonStringUri].asString();
		if (this->uri.empty())
			MS_THROW_ERROR("empty RtpHeaderExtension.uri");

		// Get the type.
		this->type = RTC::RtpHeaderExtensionUri::GetType(this->uri);

		// `preferredId` is mandatory.
		if (!data[JsonStringPreferredId].isUInt())
			MS_THROW_ERROR("missing RtpHeaderExtension.preferredId");

		this->preferredId = static_cast<uint8_t>(data[JsonStringPreferredId].asUInt());

		// `preferredEncrypt` is optional.
		if (data[JsonStringPreferredEncrypt].isBool())
			this->preferredEncrypt = data[JsonStringPreferredEncrypt].asBool();
	}

	Json::Value RtpHeaderExtension::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringUri{ "uri" };
		static const Json::StaticString JsonStringPreferredId{ "preferredId" };
		static const Json::StaticString JsonStringPreferredEncrypt{ "preferredEncrypt" };

		Json::Value json(Json::objectValue);

		// Add `kind`.
		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		// Add `uri`.
		json[JsonStringUri] = this->uri;

		// Add `preferredId`.
		json[JsonStringPreferredId] = Json::UInt{ this->preferredId };

		// Add `preferredEncrypt`.
		json[JsonStringPreferredEncrypt] = this->preferredEncrypt;

		return json;
	}
} // namespace RTC
