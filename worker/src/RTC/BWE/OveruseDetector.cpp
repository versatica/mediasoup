#define MS_CLASS "RTC::BWE::OveruseDetector"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/OveruseDetector.hpp"
#include "Logger.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Distance from the threshold beyond which it stops adapting, so that a
		// sudden capacity drop doesn't drag it along.
		static constexpr double MaxAdaptOffsetMs{ 15.0 };
		// Time the trend must stay above the threshold before declaring overuse.
		static constexpr double OverUsingTimeThresholdMs{ 10.0 };
		// Number of deltas at which the trend stops being scaled up.
		static constexpr int64_t MaxNumDeltas{ 60 };
		// Rates at which the threshold adapts upwards and downwards, per millisecond.
		static constexpr double ThresholdUpCoef{ 0.0087 };
		static constexpr double ThresholdDownCoef{ 0.039 };
		// Bounds of the adaptive threshold.
		static constexpr double ThresholdMin{ 6.0 };
		static constexpr double ThresholdMax{ 600.0 };
		// Maximum time step used when adapting the threshold.
		static constexpr double MaxThresholdUpdateDeltaMs{ 100.0 };

		/* Instance methods. */

		Types::BandwidthUsage OveruseDetector::Detect(
		  double offsetMs, double sendDeltaMs, int64_t numOfDeltas, int64_t nowUs)
		{
			MS_TRACE();

			if (numOfDeltas < 2)
			{
				return Types::BandwidthUsage::NORMAL;
			}

			// Scale the offset up with how many samples back it up, so that the
			// detector doesn't react to a couple of them.
			const double modifiedOffsetMs = std::min(numOfDeltas, MaxNumDeltas) * offsetMs;

			if (modifiedOffsetMs > this->threshold)
			{
				if (!this->timeOverUsingMs.has_value())
				{
					// Initialize the timer assuming that we have been over-using half of
					// the time since the previous sample.
					this->timeOverUsingMs = sendDeltaMs / 2;
				}
				else
				{
					this->timeOverUsingMs = this->timeOverUsingMs.value() + sendDeltaMs;
				}

				this->overuseCounter++;

				// Only declare overuse once the condition has persisted for long enough,
				// over more than a single sample, and while the trend is not decreasing
				// already. This is what filters out isolated jitter spikes.
				if (this->timeOverUsingMs.value() > OverUsingTimeThresholdMs && this->overuseCounter > 1)
				{
					if (offsetMs >= this->prevOffsetMs)
					{
						this->timeOverUsingMs = 0.0;
						this->overuseCounter  = 0;
						this->state           = Types::BandwidthUsage::OVERUSING;
					}
				}
			}
			else if (modifiedOffsetMs < -this->threshold)
			{
				this->timeOverUsingMs.reset();
				this->overuseCounter = 0;
				this->state          = Types::BandwidthUsage::UNDERUSING;
			}
			else
			{
				this->timeOverUsingMs.reset();
				this->overuseCounter = 0;
				this->state          = Types::BandwidthUsage::NORMAL;
			}

			this->prevOffsetMs = offsetMs;

			UpdateThreshold(modifiedOffsetMs, nowUs);

			return this->state;
		}

		void OveruseDetector::UpdateThreshold(double modifiedOffsetMs, int64_t nowUs)
		{
			MS_TRACE();

			if (!this->lastThresholdUpdateAtUs.has_value())
			{
				this->lastThresholdUpdateAtUs = nowUs;
			}

			// Avoid adapting the threshold to big latency spikes, caused for instance
			// by a sudden capacity drop.
			if (std::fabs(modifiedOffsetMs) > this->threshold + MaxAdaptOffsetMs)
			{
				this->lastThresholdUpdateAtUs = nowUs;

				return;
			}

			// The threshold falls faster than it rises, so that it recovers quickly
			// once the network calms down.
			const double coef =
			  std::fabs(modifiedOffsetMs) < this->threshold ? ThresholdDownCoef : ThresholdUpCoef;
			// NOTE: The coefficients above are rates per millisecond, so the step is
			// expressed in those units no matter that the instants are microseconds.
			const double elapsedMs = std::min(
			  static_cast<double>(nowUs - this->lastThresholdUpdateAtUs.value()) / 1000.0,
			  MaxThresholdUpdateDeltaMs);

			this->threshold += coef * (std::fabs(modifiedOffsetMs) - this->threshold) * elapsedMs;
			this->threshold = std::clamp(this->threshold, ThresholdMin, ThresholdMax);

			this->lastThresholdUpdateAtUs = nowUs;
		}
	} // namespace BWE
} // namespace RTC
