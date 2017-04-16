#define MS_CLASS "RTC::RtcpFeedback"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtcpFeedback::RtcpFeedback(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_type("type");
		static const Json::StaticString JsonString_parameter("parameter");

		if (!data.isObject())
			MS_THROW_ERROR("RtcpFeedback is not an object");

		// `type` is mandatory.
		if (!data[JsonString_type].isString())
			MS_THROW_ERROR("missing RtcpFeedback.type");

		this->type = data[JsonString_type].asString();

		// `parameter` is optional.
		if (data[JsonString_parameter].isString())
			this->parameter = data[JsonString_parameter].asString();
	}

	Json::Value RtcpFeedback::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_type("type");
		static const Json::StaticString JsonString_parameter("parameter");

		Json::Value json(Json::objectValue);

		// Add `type`.
		json[JsonString_type] = this->type;

		// Add `parameter` (ensure it's null if no value).
		if (this->parameter.length() > 0)
			json[JsonString_parameter] = this->parameter;
		else
			json[JsonString_parameter] = Json::nullValue;

		return json;
	}
} // namespace RTC
