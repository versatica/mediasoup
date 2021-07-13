#define MS_CLASS "RTC::ActiveSpeakerObserver"

#include "RTC/ActiveSpeakerObserver.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	constexpr uint32_t C1{ 3 };
	constexpr uint32_t C2{ 2 };
	constexpr uint32_t C3{ 0 };
	constexpr uint32_t N1{ 13 };
	constexpr uint32_t N2{ 5 };
	constexpr uint32_t N3{ 10 };
	constexpr uint32_t LevelIdleTimeout{ 40 };
	constexpr uint64_t SpeakerIdleTimeout{ 60 * 60 * 1000 };
	constexpr uint32_t LongCount{ 1 };
	constexpr uint32_t LongThreashold{ 4 };
	constexpr uint32_t MaxLevel{ 127 };
	constexpr uint32_t MinLevel{ 0 };
	constexpr uint32_t MinLevelWindowLen{ 15 * 1000 / 20 };
	constexpr double MinActivityScore{ 0.0000000001 };
	constexpr uint32_t MediumThreshold{ 7 };
	constexpr uint32_t SubunitLengthN1{ (MaxLevel - MinLevel + N1 - 1) / N1 };

	class Speaker
	{
	public:
		static constexpr uint32_t ImmediateBuffLen{ LongCount * N3 * N2 };
		static constexpr uint32_t MediumsBuffLen{ LongCount * N3 };
		static constexpr uint32_t LongsBuffLen{ LongCount };
		static constexpr uint32_t LevelsBuffLen{ LongCount * N3 * N2 };
		double immediateActivityScore{ MinActivityScore };
		double mediumActivityScore{ MinActivityScore };
		double longActivityScore{ MinActivityScore };
		uint64_t lastLevelChangeTime{ 0 };

		bool paused{ false };

		Speaker();

		void EvalActivityScores();
		double GetActivityScore(int32_t interval);
		void LevelChanged(uint32_t level, uint64_t time);
		void LevelTimedOut();

	private:
		std::array<uint8_t, ImmediateBuffLen> immediates;
		std::array<uint8_t, MediumsBuffLen> mediums;
		std::array<uint8_t, LongsBuffLen> longs;
		std::array<uint8_t, LevelsBuffLen> levels;

		uint8_t minLevel{ MinLevel };
		uint8_t nextMinLevel{ MinLevel };
		uint32_t nextMinLevelWindowLen{ 0 };

	private:
		bool ComputeImmediates();
		bool ComputeLongs();
		bool ComputeMediums();
		void EvalImmediateActivityScore();
		void EvalMediumActivityScore();
		void EvalLongActivityScore();
		void UpdateMinLevel(int8_t level);
	};

	inline int64_t BinomialCoefficient(int32_t n, int32_t r)
	{
		int32_t m = n - r;

		if (r < m)
		{
			r = m;
		}

		int64_t t = 1;
		for (int64_t i = n, j = 1; i > r; i--, j++)
		{
			t = t * i / j;
		}

		return t;
	}

	inline double ComputeActivityScore(
	  const uint8_t vL, const uint32_t nR, const double p, const double lambda)
	{
		double activityScore = std::log(BinomialCoefficient(nR, vL)) + vL * std::log(p) +
		                       (nR - vL) * std::log(1 - p) - std::log(lambda) + lambda * vL;

		if (activityScore < MinActivityScore)
		{
			activityScore = MinActivityScore;
		}

		return activityScore;
	}

	template<std::size_t L, std::size_t B>
	inline bool ComputeBigs(
	  const std::array<uint8_t, L>& littles, std::array<uint8_t, B>& bigs, uint8_t threashold)
	{
		uint32_t littleLen       = L;
		uint32_t bigLen          = B;
		uint32_t littleLenPerBig = littleLen / bigLen;
		bool changed             = false;

		for (uint32_t b = 0, l = 0; b < bigLen; b++)
		{
			uint8_t sum = 0;

			for (uint32_t lEnd = l + littleLenPerBig; l < lEnd; l++)
			{
				if (littles[l] > threashold)
				{
					sum++;
				}
			}
			if (bigs[b] != sum)
			{
				bigs[b] = sum;
				changed = true;
			}
		}
		return changed;
	}

	ActiveSpeakerObserver::ActiveSpeakerObserver(const std::string& id, json& data)
	  : RTC::RtpObserver(id)
	{
		MS_TRACE();

		auto jsonIntervalIt = data.find("interval");

		if (jsonIntervalIt == data.end() || !jsonIntervalIt->is_number())
			MS_THROW_TYPE_ERROR("missing interval");

		this->interval = jsonIntervalIt->get<int16_t>();

		if (this->interval < 100)
			this->interval = 100;
		else if (this->interval > 5000)
			this->interval = 5000;

		this->periodicTimer = new Timer(this);

		this->periodicTimer->Start(interval, interval);
	}

	ActiveSpeakerObserver::~ActiveSpeakerObserver()
	{
		MS_TRACE();

		delete this->periodicTimer;
	}

	void ActiveSpeakerObserver::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		if (producer->GetKind() != RTC::Media::Kind::AUDIO)
			MS_THROW_TYPE_ERROR("not an audio Producer");

		this->mapProducerSpeaker[producer->id].producer = producer;
		this->mapProducerSpeaker[producer->id].speaker  = new Speaker();
	}

	void ActiveSpeakerObserver::RemoveProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		if (mapProducerSpeaker[producer->id].speaker != nullptr)
		{
			delete mapProducerSpeaker[producer->id].speaker;
		}

		this->mapProducerSpeaker.erase(producer->id);

		if (producer->id == this->dominantId)
		{
			Update();
		}
	}

	void ActiveSpeakerObserver::ProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

		if (producer->GetKind() != RTC::Media::Kind::AUDIO)
			MS_THROW_TYPE_ERROR("not an audio Producer");

		this->mapProducerSpeaker[producer->id].speaker->paused = false;
	}

	void ActiveSpeakerObserver::ProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		this->mapProducerSpeaker[producer->id].speaker->paused = true;
	}

	void ActiveSpeakerObserver::ReceiveRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (IsPaused())
			return;

		uint8_t volume;
		bool voice;

		if (!packet->ReadSsrcAudioLevel(volume, voice))
			return;

		uint64_t now = DepLibUV::GetTimeMs();
		this->mapProducerSpeaker[producer->id].speaker->LevelChanged(volume, now);
	}

	void ActiveSpeakerObserver::Paused()
	{
		MS_TRACE();

		this->periodicTimer->Stop();
	}

	void ActiveSpeakerObserver::Resumed()
	{
		MS_TRACE();

		this->periodicTimer->Restart();
	}

	void ActiveSpeakerObserver::OnTimer(Timer* timer)
	{
		MS_TRACE();

		Update();
	}

	void ActiveSpeakerObserver::Update()
	{
		MS_TRACE();

		uint64_t now              = DepLibUV::GetTimeMs();
		uint64_t levelIdleTimeout = LevelIdleTimeout - (now - lastLevelIdleTime);
		if (levelIdleTimeout <= 0)
		{
			if (lastLevelIdleTime != 0)
			{
				TimeoutIdleLevels(now);
			}
			lastLevelIdleTime = now;
		}

		if (mapProducerSpeaker.size() > 0 && CalculateActiveSpeaker())
		{
			json data          = json::object();
			data["producerId"] = this->dominantId;
			std::string s      = data.dump();
			Channel::ChannelNotifier::Emit(this->id, "dominantSpeaker", data);
		}
	}

	bool ActiveSpeakerObserver::CalculateActiveSpeaker()
	{
		MS_TRACE();

		std::string newDominantId;
		int32_t speakerCount = mapProducerSpeaker.size();

		if (speakerCount == 0)
		{
			newDominantId = "";
		}
		else if (speakerCount == 1)
		{
			auto ittr     = mapProducerSpeaker.begin();
			newDominantId = ittr->second.producer->id;
		}
		else
		{
			Speaker* dominantSpeaker =
			  (dominantId.empty()) ? nullptr : mapProducerSpeaker[dominantId].speaker;

			if (dominantSpeaker == nullptr)
			{
				auto item       = mapProducerSpeaker.begin();
				newDominantId   = item->first;
				dominantSpeaker = item->second.speaker;
			}
			else
			{
				newDominantId = "";
			}

			dominantSpeaker->EvalActivityScores();
			double newDominantC2 = C2;

			for (auto itt = mapProducerSpeaker.begin(); itt != mapProducerSpeaker.end(); itt++)
			{
				RTC::Speaker* speaker = itt->second.speaker;
				const std::string id  = itt->second.producer->id;

				if (id == dominantId || speaker->paused)
				{
					continue;
				}

				speaker->EvalActivityScores();

				for (int interval = 0; interval < relativeSpeachActivitiesLen; interval++)
				{
					relativeSpeachActivities[interval] = std::log(
					  dominantSpeaker->GetActivityScore(interval) / speaker->GetActivityScore(interval));
				}

				double c1 = relativeSpeachActivities[0];
				double c2 = relativeSpeachActivities[1];
				double c3 = relativeSpeachActivities[2];

				if ((c1 > C1) && (c2 > C2) && (c3 > C3) && (c2 > newDominantC2))
				{
					newDominantC2 = c2;
					newDominantId = id;
				}
			}
		}

		if (!newDominantId.empty() && newDominantId != dominantId)
		{
			dominantId = newDominantId;
			return true;
		}

		return false;
	}

	void ActiveSpeakerObserver::TimeoutIdleLevels(uint64_t now)
	{
		MS_TRACE();

		for (auto itt = mapProducerSpeaker.begin(); itt != mapProducerSpeaker.end(); itt++)
		{
			RTC::Speaker* speaker = itt->second.speaker;
			const std::string& id = itt->second.producer->id;
			uint64_t idle         = now - speaker->lastLevelChangeTime;

			if (SpeakerIdleTimeout < idle && (dominantId.empty() || id != dominantId))
			{
				speaker->paused = true;
			}
			else if (LevelIdleTimeout < idle)
			{
				speaker->LevelTimedOut();
			}
		}
	}

	Speaker::Speaker()
	{
		MS_TRACE();

		lastLevelChangeTime = DepLibUV::GetTimeMs();
	}

	void Speaker::EvalActivityScores()
	{
		MS_TRACE();

		if (ComputeImmediates())
		{
			EvalImmediateActivityScore();
			if (ComputeMediums())
			{
				EvalMediumActivityScore();
				if (ComputeLongs())
				{
					EvalLongActivityScore();
				}
			}
		}
	}

	double Speaker::GetActivityScore(int32_t interval)
	{
		MS_TRACE();

		switch (interval)
		{
			case 0:
				return immediateActivityScore;
			case 1:
				return mediumActivityScore;
			case 2:
				return longActivityScore;
			default:
				MS_ASSERT(false, "Interval is invalid");
				break;
		}
		return 0;
	}

	void Speaker::LevelChanged(uint32_t level, uint64_t time)
	{
		if (lastLevelChangeTime <= time)
		{
			lastLevelChangeTime = time;

			int8_t b = 0;

			if (level < MinLevel)
			{
				b = MinLevel;
			}
			else if (level > MaxLevel)
			{
				b = MaxLevel;
			}
			else
			{
				b = level;
			}

			std::copy(levels.begin(), levels.end() - 1, levels.begin() + 1);

			levels[0] = b;
			UpdateMinLevel(b);
			this->paused = false;
		}
	}

	void Speaker::LevelTimedOut()
	{
		MS_TRACE();

		LevelChanged(MinLevel, lastLevelChangeTime);
	}

	bool Speaker::ComputeImmediates()
	{
		MS_TRACE();

		int8_t minLevel = this->minLevel + SubunitLengthN1;
		bool changed    = false;

		for (uint32_t i = 0; i < ImmediateBuffLen; ++i)
		{
			uint8_t level = levels[i];

			if (level < minLevel)
			{
				level = MinLevel;
			}

			uint8_t immediate = (level / SubunitLengthN1);

			if (immediates[i] != immediate)
			{
				immediates[i] = immediate;
				changed       = true;
			}
		}

		return changed;
	}

	bool Speaker::ComputeMediums()
	{
		MS_TRACE();
		return ComputeBigs<ImmediateBuffLen, MediumsBuffLen>(immediates, mediums, MediumThreshold);
	}

	bool Speaker::ComputeLongs()
	{
		MS_TRACE();
		return ComputeBigs<MediumsBuffLen, LongsBuffLen>(mediums, longs, LongThreashold);
	}

	void Speaker::EvalImmediateActivityScore()
	{
		MS_TRACE();
		immediateActivityScore = ComputeActivityScore(immediates[0], N1, 0.5, 0.78);
	}

	void Speaker::EvalMediumActivityScore()
	{
		MS_TRACE();
		mediumActivityScore = ComputeActivityScore(mediums[0], N2, 0.5, 24);
	}

	void Speaker::EvalLongActivityScore()
	{
		MS_TRACE();
		longActivityScore = ComputeActivityScore(longs[0], N3, 0.5, 47);
	}

	void Speaker::UpdateMinLevel(int8_t level)
	{
		MS_TRACE();

		if (level == MinLevel)
		{
			return;
		}

		if ((minLevel == MinLevel) || (minLevel > level))
		{
			minLevel              = level;
			nextMinLevel          = MinLevel;
			nextMinLevelWindowLen = 0;
		}
		else
		{
			if (nextMinLevel == MinLevel)
			{
				nextMinLevel          = level;
				nextMinLevelWindowLen = 1;
			}
			else
			{
				if (nextMinLevel > level)
				{
					nextMinLevel = level;
				}
				nextMinLevelWindowLen++;
				if (nextMinLevelWindowLen >= MinLevelWindowLen)
				{
					double newMinLevel = std::sqrt((double)(minLevel * nextMinLevel));

					if (newMinLevel < MinLevel)
					{
						newMinLevel = MinLevel;
					}
					else if (newMinLevel > MaxLevel)
					{
						newMinLevel = MaxLevel;
					}

					minLevel = (int8_t)newMinLevel;

					nextMinLevel          = MinLevel;
					nextMinLevelWindowLen = 0;
				}
			}
		}
	}
} // namespace RTC
