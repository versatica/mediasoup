#define MS_CLASS "RTC::TrendCalculator"
// #define MS_LOG_DEV

#include "RTC/TrendCalculator.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr float DecreaseFactorPerSecond{ 0.05f };

	TrendCalculator::TrendCalculator()
	{
		MS_TRACE();
	}

	void TrendCalculator::Update(uint32_t value, uint64_t nowMs)
	{
		MS_TRACE();

		if (this->value == 0u)
		{
			this->value                   = value;
			this->highestValue            = value;
			this->highestValueUpdatedAtMs = nowMs;

			return;
		}

		// If new value is bigger or equal than current one, use it.
		if (value >= this->value)
		{
			this->value                   = value;
			this->highestValue            = value;
			this->highestValueUpdatedAtMs = nowMs;
		}
		// Otherwise decrease current value.
		else
		{
			uint64_t elapsedTime = nowMs - this->highestValueUpdatedAtMs;

			this->value = std::max<uint32_t>(
			  value,
			  this->highestValue - (this->highestValue * DecreaseFactorPerSecond * (elapsedTime / 1000.0)));
		}
	}

	void TrendCalculator::ForceUpdate(uint32_t value, uint64_t nowMs)
	{
		MS_TRACE();

		this->value                   = value;
		this->highestValue            = value;
		this->highestValueUpdatedAtMs = nowMs;
	}
} // namespace RTC
