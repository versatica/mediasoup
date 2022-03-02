#define MS_CLASS "RTC::AudioLevelObserver"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/AudioLevelObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath> // std::lround()
#include <map>

namespace RTC
{
	/* Instance methods. */

	AudioLevelObserver::AudioLevelObserver(const std::string& id, json& data) : RTC::RtpObserver(id)
	{
		MS_TRACE();

		auto jsonMaxEntriesIt = data.find("maxEntries");

		// clang-format off
		if (
			jsonMaxEntriesIt == data.end() ||
			!Utils::Json::IsPositiveInteger(*jsonMaxEntriesIt)
		)
		// clang-format on
		{
			MS_THROW_TYPE_ERROR("missing maxEntries");
		}

		this->maxEntries = jsonMaxEntriesIt->get<uint16_t>();

		if (this->maxEntries < 1)
			MS_THROW_TYPE_ERROR("invalid maxEntries value %" PRIu16, this->maxEntries);

		auto jsonThresholdIt = data.find("threshold");

		if (jsonThresholdIt == data.end() || !jsonThresholdIt->is_number())
			MS_THROW_TYPE_ERROR("missing threshold");

		this->threshold = jsonThresholdIt->get<int8_t>();

		if (this->threshold < -127 || this->threshold > 0)
			MS_THROW_TYPE_ERROR("invalid threshold value %" PRIi8, this->threshold);

		auto jsonIntervalIt = data.find("interval");

		if (jsonIntervalIt == data.end() || !jsonIntervalIt->is_number())
			MS_THROW_TYPE_ERROR("missing interval");

		this->interval = jsonIntervalIt->get<uint16_t>();

		if (this->interval < 250)
			this->interval = 250;
		else if (this->interval > 5000)
			this->interval = 5000;

		this->periodicTimer = new Timer(this);

		this->periodicTimer->Start(this->interval, this->interval);
	}

	AudioLevelObserver::~AudioLevelObserver()
	{
		MS_TRACE();

		delete this->periodicTimer;
	}

	void AudioLevelObserver::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		if (producer->GetKind() != RTC::Media::Kind::AUDIO)
			MS_THROW_TYPE_ERROR("not an audio Producer");

		// Insert into the map.
		this->mapProducerDBovs[producer];
	}

	void AudioLevelObserver::RemoveProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove from the map.
		this->mapProducerDBovs.erase(producer);
	}

	void AudioLevelObserver::ReceiveRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (IsPaused())
			return;

		uint8_t volume;
		bool voice;

		if (!packet->ReadSsrcAudioLevel(volume, voice))
			return;

		auto& dBovs = this->mapProducerDBovs.at(producer);

		dBovs.totalSum += volume;
		dBovs.count++;
	}

	void AudioLevelObserver::ProducerPaused(RTC::Producer* producer)
	{
		// Remove from the map.
		this->mapProducerDBovs.erase(producer);
	}

	void AudioLevelObserver::ProducerResumed(RTC::Producer* producer)
	{
		// Insert into the map.
		this->mapProducerDBovs[producer];
	}

	void AudioLevelObserver::Paused()
	{
		MS_TRACE();

		this->periodicTimer->Stop();

		ResetMapProducerDBovs();

		if (!this->silence)
		{
			this->silence = true;

			Channel::ChannelNotifier::Emit(this->id, "silence");
		}
	}

	void AudioLevelObserver::Resumed()
	{
		MS_TRACE();

		this->periodicTimer->Restart();
	}

	void AudioLevelObserver::Update()
	{
		MS_TRACE();

		absl::btree_map<int8_t, RTC::Producer*> mapDBovsProducer;

		for (auto& kv : this->mapProducerDBovs)
		{
			auto* producer = kv.first;
			auto& dBovs    = kv.second;

			if (dBovs.count < 10)
				continue;

			auto avgDBov = -1 * static_cast<int8_t>(std::lround(dBovs.totalSum / dBovs.count));

			if (avgDBov >= this->threshold)
				mapDBovsProducer[avgDBov] = producer;
		}

		// Clear the map.
		ResetMapProducerDBovs();

		if (!mapDBovsProducer.empty())
		{
			this->silence = false;

			uint16_t idx{ 0 };
			auto rit  = mapDBovsProducer.crbegin();
			json data = json::array();

			for (; idx < this->maxEntries && rit != mapDBovsProducer.crend(); ++idx, ++rit)
			{
				data.emplace_back(json::value_t::object);

				auto& jsonEntry = data[idx];

				jsonEntry["producerId"] = rit->second->id;
				jsonEntry["volume"]     = rit->first;
			}

			Channel::ChannelNotifier::Emit(this->id, "volumes", data);
		}
		else if (!this->silence)
		{
			this->silence = true;

			Channel::ChannelNotifier::Emit(this->id, "silence");
		}
	}

	void AudioLevelObserver::ResetMapProducerDBovs()
	{
		MS_TRACE();

		for (auto& kv : this->mapProducerDBovs)
		{
			auto& dBovs = kv.second;

			dBovs.totalSum = 0;
			dBovs.count    = 0;
		}
	}

	inline void AudioLevelObserver::OnTimer(Timer* /*timer*/)
	{
		MS_TRACE();

		Update();
	}
} // namespace RTC
