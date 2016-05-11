#define MS_CLASS "RTC::RtcpParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtcpParameters::RtcpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_cname("cname");
		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_reducedSize("reducedSize");

		if (!data.isObject())
			MS_THROW_ERROR("`RtcpParameters` is not an object");

		// `cname` is optional.
		if (data[k_cname].isString())
		{
			this->cname = data[k_cname].asString();
			if (this->cname.empty())
				MS_THROW_ERROR("empty `RtcpParameters.cname`");
		}

		// `ssrc` is optional.
		if (data[k_ssrc].isUInt())
			this->ssrc = (uint32_t)data[k_ssrc].asUInt();

		// `reducedSize` is optional.
		if (data[k_reducedSize].isBool())
			this->reducedSize = data[k_reducedSize].asBool();
	}

	RtcpParameters::~RtcpParameters()
	{
		MS_TRACE();
	}

	Json::Value RtcpParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_cname("cname");
		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_reducedSize("reducedSize");

		Json::Value json(Json::objectValue);

		// Add `cname`.
		if (!this->cname.empty())
			json[k_cname] = this->cname;

		// Add `ssrc`.
		if (this->ssrc)
			json[k_ssrc] = (Json::UInt)this->ssrc;

		// Add `reducedSize`.
		json[k_reducedSize] = this->reducedSize;

		return json;
	}
}
