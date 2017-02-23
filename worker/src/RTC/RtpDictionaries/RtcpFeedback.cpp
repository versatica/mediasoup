#define MS_CLASS "RTC::RtcpFeedback"
// #define MS_LOG_DEV

#include "RTC/RtpDictionaries.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RtcpFeedback::RtcpFeedback(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_type("type");
		static const Json::StaticString k_parameter("parameter");

		if (!data.isObject())
			MS_THROW_ERROR("RtcpFeedback is not an object");

		// `type` is mandatory.
		if (!data[k_type].isString())
			MS_THROW_ERROR("missing RtcpFeedback.type");

		this->type = data[k_type].asString();

		// `parameter` is optional.
		if (data[k_parameter].isString())
			this->parameter = data[k_parameter].asString();
	}

	Json::Value RtcpFeedback::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_type("type");
		static const Json::StaticString k_parameter("parameter");

		Json::Value json(Json::objectValue);

		// Add `type`.
		json[k_type] = this->type;

		// Add `parameter` (ensure it's null if no value).
		if (this->parameter.length() > 0)
			json[k_parameter] = this->parameter;
		else
			json[k_parameter] = Json::nullValue;

		return json;
	}
}
