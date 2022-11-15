#define MS_CLASS "RTC::RtcpFeedback"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtcpFeedback::RtcpFeedback(const FBS::RtpParameters::RtcpFeedback* data)
	{
		MS_TRACE();

		this->type = data->type()->str();

		// parameter is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtcpFeedback::VT_PARAMETER))
			this->parameter = data->parameter()->str();
	}

	void RtcpFeedback::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add type.
		jsonObject["type"] = this->type;

		// Add parameter (optional).
		if (this->parameter.length() > 0)
			jsonObject["parameter"] = this->parameter;
	}
} // namespace RTC
