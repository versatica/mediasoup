#define MS_CLASS "RTC::SctpStreamParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SctpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	SctpStreamParameters::SctpStreamParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_TYPE_ERROR("data is not an object");

		auto jsonStreamIdIt = data.find("streamId");

		// streamId is mandatory.
		if (jsonStreamIdIt == data.end() || !jsonStreamIdIt->is_number_unsigned())
			MS_THROW_TYPE_ERROR("missing streamId");

		this->streamId = jsonStreamIdIt->get<uint16_t>();

		// TODO: Validate streamId range.

		// TODO: More.
	}

	void SctpStreamParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add streamId.
		jsonObject["streamId"] = this->streamId;

		// TODO: More.
	}
} // namespace RTC
