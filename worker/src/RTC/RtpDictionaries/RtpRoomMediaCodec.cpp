#define MS_CLASS "RTC::RtpRoomMediaCodec"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpRoomMediaCodec::RtpRoomMediaCodec(Json::Value& data) :
		RtpCodec(data)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");

		// `kind` is mandatory.
		if (!data[k_kind].isString())
			MS_THROW_ERROR("missing RtpRoomMediaCodec.kind");

		std::string kind = data[k_kind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// Validate codec.

		// It must not be a feature codec.
		if (this->mime.IsFeatureCodec())
			MS_THROW_ERROR("RtpRoomMediaCodec can not be a feature coedc");
	}
}
