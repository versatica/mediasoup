#define MS_CLASS "RTC::RttCalculator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RttCalculator.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	// How long a measurement counts towards the average (ms).
	constexpr int64_t MeasurementWindowMs{ 1500 };
	// How often the average is worked out again even if nothing was measured,
	// since a measurement ageing out moves it just as much (ms).
	constexpr int64_t UpdateIntervalMs{ 1000 };
	// How much of the average the measurements of the window make up, the rest
	// being the previous average.
	constexpr float WeightFactor{ 0.3f };

	/* Instance methods. */

	RttCalculator::RttCalculator(Listener* listener, SharedInterface* shared)
	  : listener(listener),
	    shared(shared),
	    updateTimer(shared->CreateTimer(this, "rtt-calculator-update"))
	{
		MS_TRACE();
	}

	void RttCalculator::AddRtt(int64_t rttMs, int64_t nowMs)
	{
		MS_TRACE();

		if (!this->updateTimer->IsActive())
		{
			this->updateTimer->Start(UpdateIntervalMs, UpdateIntervalMs);
		}

		this->measurements.push_back({ .rttMs = rttMs, .atMs = nowMs });

		UpdateAndReport(nowMs);
	}

	void RttCalculator::UpdateAndReport(int64_t nowMs)
	{
		MS_TRACE();

		std::erase_if(
		  this->measurements,
		  [nowMs](const Measurement& measurement) -> bool
		  {
			  return nowMs - measurement.atMs > MeasurementWindowMs;
		  });

		// Nothing measured recently enough to say anything with, so the average is
		// forgotten and the listener is left with the last one it was told.
		if (this->measurements.empty())
		{
			this->avgRttMs.reset();

			this->updateTimer->Stop();

			return;
		}

		int64_t sumRttMs{ 0 };

		for (const auto& measurement : this->measurements)
		{
			sumRttMs += measurement.rttMs;
		}

		const int64_t windowRttMs = sumRttMs / static_cast<int64_t>(this->measurements.size());

		if (!this->avgRttMs.has_value())
		{
			this->avgRttMs = windowRttMs;
		}
		else
		{
			this->avgRttMs = static_cast<int64_t>(
			  (static_cast<float>(this->avgRttMs.value()) * (1.0f - WeightFactor)) +
			  (static_cast<float>(windowRttMs) * WeightFactor));
		}

		this->lastRttMs = this->avgRttMs;

		this->listener->OnRttCalculatorRttUpdated(this, this->avgRttMs.value());
	}

	void RttCalculator::OnTimer(TimerHandleInterface* timer)
	{
		MS_TRACE();

		if (timer == this->updateTimer.get())
		{
			UpdateAndReport(this->shared->GetTimeMs());
		}
	}
} // namespace RTC
