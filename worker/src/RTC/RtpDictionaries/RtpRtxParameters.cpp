#define MS_CLASS "RTC::RtpRtxParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpRtxParameters::RtpRtxParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_TYPE_ERROR("data is not an object");

		auto jsonSsrcIt = data.find("ssrc");

		// ssrc is optional.
		if (jsonSsrcIt != data.end() && jsonSsrcIt->is_number_unsigned())
			this->ssrc = jsonSsrcIt->get<uint32_t>();
	}

	void RtpRtxParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Force it to be an object even if no key/values are added below.
		jsonObject = json::object();

		// Add ssrc (optional).
		if (this->ssrc != 0u)
			jsonObject["ssrc"] = this->ssrc;
	}
} // namespace RTC
