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

		static const Json::StaticString JsonStringType{ "type" };
		static const Json::StaticString JsonStringParameter{ "parameter" };

		if (!data.isObject())
			MS_THROW_ERROR("RtcpFeedback is not an object");

		// `type` is mandatory.
		if (!data[JsonStringType].isString())
			MS_THROW_ERROR("missing RtcpFeedback.type");

		this->type = data[JsonStringType].asString();

		// `parameter` is optional.
		if (data[JsonStringParameter].isString())
			this->parameter = data[JsonStringParameter].asString();
	}

	Json::Value RtcpFeedback::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringType{ "type" };
		static const Json::StaticString JsonStringParameter{ "parameter" };

		Json::Value json(Json::objectValue);

		// Add `type`.
		json[JsonStringType] = this->type;

		// Add `parameter` (ensure it's null if no value).
		if (this->parameter.length() > 0)
			json[JsonStringParameter] = this->parameter;
		else
			json[JsonStringParameter] = Json::nullValue;

		return json;
	}
} // namespace RTC
