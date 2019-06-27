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

		auto jsonStreamIdIt          = data.find("streamId");
		auto jsonOrderedIdIt         = data.find("ordered");
		auto jsonMaxPacketLifeTimeIt = data.find("maxPacketLifeTime");
		auto jsonMaxRetransmitsIt    = data.find("maxRetransmits");

		// streamId is mandatory.
		if (jsonStreamIdIt == data.end() || !jsonStreamIdIt->is_number_unsigned())
			MS_THROW_TYPE_ERROR("missing streamId");

		this->streamId = jsonStreamIdIt->get<uint16_t>();

		// ordered is optional.
		if (jsonOrderedIdIt != data.end() && jsonOrderedIdIt->is_number_unsigned())
			this->ordered = jsonOrderedIdIt->get<bool>();

		// maxPacketLifeTime is optional.
		if (jsonMaxPacketLifeTimeIt != data.end() && jsonMaxPacketLifeTimeIt->is_number_unsigned())
			this->maxPacketLifeTime = jsonMaxPacketLifeTimeIt->get<uint32_t>();

		// maxRetransmits is optional.
		if (jsonMaxRetransmitsIt != data.end() && jsonMaxRetransmitsIt->is_number_unsigned())
			this->maxRetransmits = jsonMaxRetransmitsIt->get<uint32_t>();
	}

	void SctpStreamParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add streamId.
		jsonObject["streamId"] = this->streamId;

		// Add ordered.
		jsonObject["ordered"] = this->ordered;

		// Add maxPacketLifeTime.
		jsonObject["maxPacketLifeTime"] = this->maxPacketLifeTime;

		// Add maxRetransmits.
		jsonObject["maxRetransmits"] = this->maxRetransmits;
	}
} // namespace RTC
