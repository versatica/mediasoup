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

		static const Json::StaticString JsonStringCname{ "cname" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringReducedSize{ "reducedSize" };

		if (!data.isObject())
			MS_THROW_ERROR("RtcpParameters is not an object");

		// `cname` is optional.
		if (data[JsonStringCname].isString())
		{
			this->cname = data[JsonStringCname].asString();
			if (this->cname.empty())
				MS_THROW_ERROR("empty RtcpParameters.cname");
		}

		// `ssrc` is optional.
		if (data[JsonStringSsrc].isUInt())
			this->ssrc = uint32_t{ data[JsonStringSsrc].asUInt() };

		// `reducedSize` is optional.
		if (data[JsonStringReducedSize].isBool())
			this->reducedSize = data[JsonStringReducedSize].asBool();
	}

	Json::Value RtcpParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringCname{ "cname" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringReducedSize{ "reducedSize" };

		Json::Value json(Json::objectValue);

		// Add `cname`.
		if (!this->cname.empty())
			json[JsonStringCname] = this->cname;

		// Add `ssrc`.
		if (this->ssrc != 0u)
			json[JsonStringSsrc] = Json::UInt{ this->ssrc };

		// Add `reducedSize`.
		json[JsonStringReducedSize] = this->reducedSize;

		return json;
	}
} // namespace RTC
