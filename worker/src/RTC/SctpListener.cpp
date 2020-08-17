#define MS_CLASS "RTC::SctpListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SctpListener.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/DataProducer.hpp"

namespace RTC
{
	/* Instance methods. */

	void SctpListener::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		jsonObject["streamIdTable"] = json::object();

		auto jsonStreamIdTableIt = jsonObject.find("streamIdTable");

		// Add streamIdTable.
		for (const auto& kv : this->streamIdTable)
		{
			auto streamId      = kv.first;
			auto* dataProducer = kv.second;

			(*jsonStreamIdTableIt)[std::to_string(streamId)] = dataProducer->id;
		}
	}

	void SctpListener::AddDataProducer(RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		const auto& sctpParameters = dataProducer->GetSctpStreamParameters();

		// Add entries into the streamIdTable.
		auto streamId = sctpParameters.streamId;

		if (this->streamIdTable.find(streamId) == this->streamIdTable.end())
		{
			this->streamIdTable[streamId] = dataProducer;
		}
		else
		{
			MS_THROW_ERROR("streamId already exists in SCTP listener [streamId:%" PRIu16 "]", streamId);
		}
	}

	void SctpListener::RemoveDataProducer(RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		// Remove from the listener table all entries pointing to the DataProducer.
		for (auto it = this->streamIdTable.begin(); it != this->streamIdTable.end();)
		{
			if (it->second == dataProducer)
				it = this->streamIdTable.erase(it);
			else
				++it;
		}
	}

	RTC::DataProducer* SctpListener::GetDataProducer(uint16_t streamId)
	{
		MS_TRACE();

		auto it = this->streamIdTable.find(streamId);

		if (it != this->streamIdTable.end())
		{
			auto* dataProducer = it->second;

			return dataProducer;
		}

		return nullptr;
	}
} // namespace RTC
