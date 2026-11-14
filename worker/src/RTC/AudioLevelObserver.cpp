#define MS_CLASS "RTC::AudioLevelObserver"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/AudioLevelObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath> // std::lround()
#include <map>   // std::multimap
#include <vector>

namespace RTC
{
	/* Instance methods. */

	AudioLevelObserver::AudioLevelObserver(
	  SharedInterface* shared,
	  const std::string& id,
	  RTC::RtpObserver::Listener* listener,
	  const FBS::AudioLevelObserver::AudioLevelObserverOptions* options)
	  : RTC::RtpObserver(shared, id, listener),
	    maxEntries(options->maxEntries()),
	    threshold(options->threshold()),
	    intervalMs(std::clamp<int64_t>(options->interval(), 250, 5000)),
	    periodicTimer(shared->CreateTimer(this, "audio-level-observer"))
	{
		MS_TRACE();

		if (this->threshold > 0)
		{
			MS_THROW_TYPE_ERROR("invalid threshold value %" PRIi8, this->threshold);
		}

		this->periodicTimer->Start(this->intervalMs, this->intervalMs);

		// NOTE: This may throw.
		this->shared->GetChannelMessageRegistrator()->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*channelNotificationHandler*/ nullptr);
	}

	AudioLevelObserver::~AudioLevelObserver()
	{
		MS_TRACE();

		this->shared->GetChannelMessageRegistrator()->UnregisterHandler(this->id);
	}

	void AudioLevelObserver::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		if (producer->GetKind() != RTC::Media::Kind::AUDIO)
		{
			MS_THROW_TYPE_ERROR("not an audio Producer");
		}

		// Insert into the map.
		this->mapProducerDBovs[producer];
	}

	void AudioLevelObserver::RemoveProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove from the map.
		this->mapProducerDBovs.erase(producer);
	}

	void AudioLevelObserver::ReceiveRtpPacket(RTC::Producer* producer, RTC::RTP::Packet* packet)
	{
		MS_TRACE();

		if (IsPaused())
		{
			return;
		}

		uint8_t volume{ 0 };
		bool voice{ false };

		if (!packet->ReadSsrcAudioLevel(volume, voice))
		{
			return;
		}

		auto& dBovs = this->mapProducerDBovs.at(producer);

		dBovs.totalSum += volume;
		dBovs.count++;
	}

	void AudioLevelObserver::ProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove from the map.
		this->mapProducerDBovs.erase(producer);
	}

	void AudioLevelObserver::ProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

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

			this->shared->GetChannelNotifier()->Emit(
			  this->id, FBS::Notification::Event::AUDIOLEVELOBSERVER_SILENCE);
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

		std::multimap<int8_t, RTC::Producer*> mapDBovsProducer;

		for (const auto& kv : this->mapProducerDBovs)
		{
			auto* const producer = kv.first;
			const auto& dBovs    = kv.second;

			if (dBovs.count < 10)
			{
				continue;
			}

			const auto avgDBov = static_cast<int8_t>(-std::lround(dBovs.totalSum / dBovs.count));

			if (avgDBov >= this->threshold)
			{
				mapDBovsProducer.insert({ avgDBov, producer });
			}
		}

		// Clear the map.
		ResetMapProducerDBovs();

		if (!mapDBovsProducer.empty())
		{
			this->silence = false;

			uint16_t idx{ 0 };
			auto rit = mapDBovsProducer.crbegin();

			std::vector<flatbuffers::Offset<FBS::AudioLevelObserver::Volume>> volumes;

			for (; idx < this->maxEntries && rit != mapDBovsProducer.crend(); ++idx, ++rit)
			{
				const int8_t avgDBov = rit->first;
				const auto* producer = rit->second;

				volumes.emplace_back(
				  FBS::AudioLevelObserver::CreateVolumeDirect(
				    this->shared->GetChannelNotifier()->GetBufferBuilder(), producer->id.c_str(), avgDBov));
			}

			const auto notification = FBS::AudioLevelObserver::CreateVolumesNotificationDirect(
			  this->shared->GetChannelNotifier()->GetBufferBuilder(), std::addressof(volumes));

			this->shared->GetChannelNotifier()->Emit(
			  this->id,
			  FBS::Notification::Event::AUDIOLEVELOBSERVER_VOLUMES,
			  FBS::Notification::Body::AudioLevelObserver_VolumesNotification,
			  notification);
		}
		else if (!this->silence)
		{
			this->silence = true;

			this->shared->GetChannelNotifier()->Emit(
			  this->id, FBS::Notification::Event::AUDIOLEVELOBSERVER_SILENCE);
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

	inline void AudioLevelObserver::OnTimer(TimerHandleInterface* /*timer*/)
	{
		MS_TRACE();

		Update();
	}
} // namespace RTC
