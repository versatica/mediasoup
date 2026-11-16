#ifndef MS_RTC_AUDIO_LEVEL_OBSERVER_HPP
#define MS_RTC_AUDIO_LEVEL_OBSERVER_HPP

#include "common.hpp"
#include "FBS/audioLevelObserver.h"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/RtpObserver.hpp"
#include "SharedInterface.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	class AudioLevelObserver : public RTC::RtpObserver, public TimerHandleInterface::Listener
	{
	private:
		struct DBovs
		{
			/**
			 * Sum of dBovs (positive integer).
			 */
			uint16_t totalSum{ 0 };
			/**
			 * Number of dBovs entries in totalSum.
			 */
			size_t count{ 0 };
		};

	public:
		AudioLevelObserver(
		  SharedInterface* shared,
		  const std::string& id,
		  RTC::RtpObserver::Listener* listener,
		  const FBS::AudioLevelObserver::AudioLevelObserverOptions* options);

		~AudioLevelObserver() override;

	public:
		void AddProducer(RTC::Producer* producer) override;

		void RemoveProducer(RTC::Producer* producer) override;

		void ReceiveRtpPacket(RTC::Producer* producer, RTC::RTP::Packet* packet) override;

		void ProducerPaused(RTC::Producer* producer) override;

		void ProducerResumed(RTC::Producer* producer) override;

	private:
		void Paused() override;

		void Resumed() override;

		void Update();

		void ResetMapProducerDBovs();

		/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
	protected:
		void OnTimer(TimerHandleInterface* timer) override;

	private:
		// Passed by argument.
		const uint16_t maxEntries;
		const int8_t threshold;
		const int64_t intervalMs;
		// Allocated by this.
		const std::unique_ptr<TimerHandleInterface> periodicTimer;
		// Others.
		ankerl::unordered_dense::map<RTC::Producer*, DBovs> mapProducerDBovs;
		bool silence{ true };
	};
} // namespace RTC

#endif
