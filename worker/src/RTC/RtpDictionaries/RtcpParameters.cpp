#define MS_CLASS "RTC::RtcpParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtcpParameters::RtcpParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_TYPE_ERROR("data is not an object");

		auto jsonCnameIt       = data.find("cname");
		auto jsonSsrcIt        = data.find("ssrc");
		auto jsonRedicedSizeIt = data.find("reducedSize");

		// cname is optional.
		if (jsonCnameIt != data.end() && jsonCnameIt->is_string())
			this->cname = jsonCnameIt->get<std::string>();

		// ssrc is optional.
		// clang-format off
		if (
			jsonSsrcIt != data.end() &&
			Utils::Json::IsPositiveInteger(*jsonSsrcIt)
		)
		// clang-format on
		{
			this->ssrc = jsonSsrcIt->get<uint32_t>();
		}

		// reducedSize is optional.
		if (jsonRedicedSizeIt != data.end() && jsonRedicedSizeIt->is_boolean())
			this->reducedSize = jsonRedicedSizeIt->get<bool>();
	}

	void RtcpParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add cname.
		if (!this->cname.empty())
			jsonObject["cname"] = this->cname;

		// Add ssrc.
		if (this->ssrc != 0u)
			jsonObject["ssrc"] = this->ssrc;

		// Add reducedSize.
		jsonObject["reducedSize"] = this->reducedSize;
	}
} // namespace RTC
