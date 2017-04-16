#define MS_CLASS "RTC::RtcpParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtcpParameters::RtcpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_cname("cname");
		static const Json::StaticString JsonString_ssrc("ssrc");
		static const Json::StaticString JsonString_reducedSize("reducedSize");

		if (!data.isObject())
			MS_THROW_ERROR("RtcpParameters is not an object");

		// `cname` is optional.
		if (data[JsonString_cname].isString())
		{
			this->cname = data[JsonString_cname].asString();
			if (this->cname.empty())
				MS_THROW_ERROR("empty RtcpParameters.cname");
		}

		// `ssrc` is optional.
		if (data[JsonString_ssrc].isUInt())
			this->ssrc = (uint32_t)data[JsonString_ssrc].asUInt();

		// `reducedSize` is optional.
		if (data[JsonString_reducedSize].isBool())
			this->reducedSize = data[JsonString_reducedSize].asBool();
	}

	Json::Value RtcpParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_cname("cname");
		static const Json::StaticString JsonString_ssrc("ssrc");
		static const Json::StaticString JsonString_reducedSize("reducedSize");

		Json::Value json(Json::objectValue);

		// Add `cname`.
		if (!this->cname.empty())
			json[JsonString_cname] = this->cname;

		// Add `ssrc`.
		if (this->ssrc)
			json[JsonString_ssrc] = (Json::UInt)this->ssrc;

		// Add `reducedSize`.
		json[JsonString_reducedSize] = this->reducedSize;

		return json;
	}
} // namespace RTC
