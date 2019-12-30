#define MS_CLASS "RTC::SctpStreamParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SctpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	SctpStreamParameters::SctpStreamParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_TYPE_ERROR("data is not an object");

		auto jsonStreamIdIt          = data.find("streamId");
		auto jsonOrderedIdIt         = data.find("ordered");
		auto jsonMaxPacketLifeTimeIt = data.find("maxPacketLifeTime");
		auto jsonMaxRetransmitsIt    = data.find("maxRetransmits");

		// streamId is mandatory.
		if (jsonStreamIdIt == data.end() || !Utils::Json::IsPositiveInteger(*jsonStreamIdIt))
			MS_THROW_TYPE_ERROR("missing streamId");

		this->streamId = jsonStreamIdIt->get<uint16_t>();

		if (this->streamId > 65534)
			MS_THROW_TYPE_ERROR("streamId must not be greater than 65534");

		// ordered is optional.
		bool orderedGiven = false;

		if (jsonOrderedIdIt != data.end() && jsonOrderedIdIt->is_boolean())
		{
			orderedGiven  = true;
			this->ordered = jsonOrderedIdIt->get<bool>();
		}

		// maxPacketLifeTime is optional.
		// clang-format off
		if (
			jsonMaxPacketLifeTimeIt != data.end() &&
			Utils::Json::IsPositiveInteger(*jsonMaxPacketLifeTimeIt)
		)
		// clang-format on
		{
			this->maxPacketLifeTime = jsonMaxPacketLifeTimeIt->get<uint16_t>();
		}

		// maxRetransmits is optional.
		// clang-format off
		if (
			jsonMaxRetransmitsIt != data.end() &&
			Utils::Json::IsPositiveInteger(*jsonMaxRetransmitsIt)
		)
		// clang-format on
		{
			this->maxRetransmits = jsonMaxRetransmitsIt->get<uint16_t>();
		}

		if (this->maxPacketLifeTime && this->maxRetransmits)
			MS_THROW_TYPE_ERROR("cannot provide both maxPacketLifeTime and maxRetransmits");

		// clang-format off
		if (
			orderedGiven &&
			this->ordered &&
			(this->maxPacketLifeTime || this->maxRetransmits)
		)
		// clang-format on
		{
			MS_THROW_TYPE_ERROR("cannot be ordered with maxPacketLifeTime or maxRetransmits");
		}
		else if (!orderedGiven && (this->maxPacketLifeTime || this->maxRetransmits))
		{
			this->ordered = false;
		}
	}

	void SctpStreamParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add streamId.
		jsonObject["streamId"] = this->streamId;

		// Add ordered.
		jsonObject["ordered"] = this->ordered;

		// Add maxPacketLifeTime.
		if (this->maxPacketLifeTime)
			jsonObject["maxPacketLifeTime"] = this->maxPacketLifeTime;

		// Add maxRetransmits.
		if (this->maxRetransmits)
			jsonObject["maxRetransmits"] = this->maxRetransmits;
	}
} // namespace RTC
