#define MS_CLASS "RTC::RtpRoomMediaCodec"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpRoomMediaCodec::RtpRoomMediaCodec(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_parameters("parameters");

		if (!data.isObject())
			MS_THROW_ERROR("RtpRoomMediaCodec is not an object");

		// `kind` is mandatory.
		if (!data[k_kind].isString())
			MS_THROW_ERROR("missing RtpRoomMediaCodec.kind");

		std::string kind = data[k_kind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing RtpRoomMediaCodec.name");

		std::string name = data[k_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		// `clockRate` is mandatory.
		if (!data[k_clockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.clockRate");

		this->clockRate = (uint32_t)data[k_clockRate].asUInt();

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			RTC::FillCustomParameters(this->parameters, data[k_parameters]);

		// Validate codec.

		// It must not be a feature codec.
		if (this->mime.IsFeatureCodec())
			MS_THROW_ERROR("RtpRoomMediaCodec can not be a feature coedc");
	}
}
