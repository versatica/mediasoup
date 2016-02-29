#define MS_CLASS "RTC::RtcpFeedback"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

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
			MS_THROW_ERROR("missing `RtcpFeedback.type`");

		this->type = data[k_type].asString();

		// `parameter` is optional.
		if (data[k_parameter].isString())
			this->parameter = data[k_parameter].asString();
	}

	RtcpFeedback::~RtcpFeedback()
	{
		MS_TRACE();
	}

	Json::Value RtcpFeedback::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_type("type");
		static const Json::StaticString k_parameter("parameter");

		Json::Value json(Json::objectValue);

		// Add `type`.
		json[k_type] = this->type;

		// Add `parameter`.
		json[k_parameter] = this->parameter;

		return json;
	}
}
