#define MS_CLASS "RTC::BWE::TrendlineEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/TrendlineEstimator.hpp"
#include "Logger.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Coefficient of the exponential filter applied to the accumulated delay.
		static constexpr double SmoothingCoef{ 0.9 };
		// Gain applied to the slope before comparing it against the threshold.
		static constexpr double ThresholdGain{ 4.0 };
		// Number of deltas at which the modified trend stops being scaled down.
		static constexpr int MinNumDeltas{ 60 };
		// Maximum value of the delta counter.
		static constexpr int DeltaCounterMax{ 1000 };
		// Time the trend must stay above the threshold before declaring overuse.
		static constexpr double OverusingTimeThresholdMs{ 10 };
		// Rates at which the threshold adapts upwards and downwards, per millisecond.
		static constexpr double ThresholdUpCoef{ 0.0087 };
		static constexpr double ThresholdDownCoef{ 0.039 };
		// Bounds of the adaptive threshold.
		static constexpr double ThresholdMin{ 6.0 };
		static constexpr double ThresholdMax{ 600.0 };
		// Distance from the threshold beyond which it stops adapting, so that a
		// sudden capacity drop doesn't drag it along.
		//
		// NOTE: It is not a time, being compared against the trend, which is a slope
		// and hence dimensionless.
		static constexpr double MaxAdaptOffset{ 15.0 };
		// Maximum time step used when adapting the threshold.
		static constexpr int64_t MaxThresholdUpdateDeltaMs{ 100 };

		/* Instance methods. */

		TrendlineEstimator::TrendlineEstimator(size_t windowSize) : windowSize(windowSize)
		{
			MS_TRACE();

			MS_ASSERT(windowSize >= 2, "window size must be at least 2 [windowSize:%zu]", windowSize);
		}

		void TrendlineEstimator::Update(int64_t sendDeltaUs, int64_t arrivalDeltaUs, int64_t arrivalTimeUs)
		{
			MS_TRACE();

			const auto deltaUs = static_cast<double>(arrivalDeltaUs - sendDeltaUs);

			this->numOfDeltas = std::min(this->numOfDeltas + 1, DeltaCounterMax);

			if (!this->firstArrivalTimeUs.has_value())
			{
				this->firstArrivalTimeUs = arrivalTimeUs;
			}

			// Exponential backoff filter.
			this->accumulatedDelayUs += deltaUs;
			this->smoothedDelayUs =
			  (SmoothingCoef * this->smoothedDelayUs) + ((1 - SmoothingCoef) * this->accumulatedDelayUs);

			// Maintain the samples window. A group may arrive before the first one of
			// the window did, in which case the regression just gets a negative x.
			const auto elapsedUs = static_cast<double>(arrivalTimeUs - this->firstArrivalTimeUs.value());

			this->delayHist.push_back({ elapsedUs, this->smoothedDelayUs });

			if (this->delayHist.size() > this->windowSize)
			{
				this->delayHist.pop_front();
			}

			// Simple linear regression. The slope can be seen as an estimate of
			// (sendRate - capacity) / capacity:
			//   0 < trend < 1  ->  the delay increases, queues are filling up.
			//   trend == 0     ->  the delay does not change.
			//   trend < 0      ->  the delay decreases, queues are being emptied.
			double trend = this->prevTrend;

			if (this->delayHist.size() == this->windowSize)
			{
				// Keep the previous trend if the line cannot be fitted.
				trend = GetLinearFitSlope().value_or(trend);
			}

			Detect(trend, static_cast<double>(sendDeltaUs), arrivalTimeUs);
		}

		std::optional<double> TrendlineEstimator::GetLinearFitSlope() const
		{
			MS_TRACE();

			MS_ASSERT(
			  this->delayHist.size() >= 2,
			  "not enough samples to fit a line [samples:%zu]",
			  this->delayHist.size());

			// Compute the "center of mass".
			double sumX{ 0 };
			double sumY{ 0 };

			for (const auto& sample : this->delayHist)
			{
				sumX += sample.arrivalTimeUs;
				sumY += sample.smoothedDelayUs;
			}

			const double avgX = sumX / this->delayHist.size();
			const double avgY = sumY / this->delayHist.size();

			// Compute the slope as sum((x - avgX) * (y - avgY)) / sum((x - avgX)^2).
			double numerator{ 0 };
			double denominator{ 0 };

			for (const auto& sample : this->delayHist)
			{
				const double x = sample.arrivalTimeUs - avgX;
				const double y = sample.smoothedDelayUs - avgY;

				numerator += x * y;
				denominator += x * x;
			}

			// The line cannot be fitted when every sample arrived at the very same
			// time.
			if (denominator == 0)
			{
				return std::nullopt;
			}

			return numerator / denominator;
		}

		void TrendlineEstimator::Detect(double trend, double sendDeltaUs, int64_t arrivalTimeUs)
		{
			MS_TRACE();

			if (this->numOfDeltas < 2)
			{
				this->state = Types::BandwidthUsage::NORMAL;

				return;
			}

			// Scale the trend down while there are few deltas, so that the estimator
			// doesn't react to a couple of samples.
			const double modifiedTrend = std::min(this->numOfDeltas, MinNumDeltas) * trend * ThresholdGain;

			if (modifiedTrend > this->threshold)
			{
				if (!this->timeOverUsingUs.has_value())
				{
					// Initialize the timer assuming that we have been over-using half of
					// the time since the previous sample.
					this->timeOverUsingUs = sendDeltaUs / 2;
				}
				else
				{
					this->timeOverUsingUs = this->timeOverUsingUs.value() + sendDeltaUs;
				}

				this->overuseCounter++;

				// Only declare overuse once the condition has persisted for long enough,
				// over more than a single sample, and while the trend is not decreasing
				// already. This is what filters out isolated jitter spikes.
				if (this->timeOverUsingUs.value() / 1000.0 > OverusingTimeThresholdMs && this->overuseCounter > 1)
				{
					if (trend >= this->prevTrend)
					{
						this->timeOverUsingUs = 0;
						this->overuseCounter  = 0;
						this->state           = Types::BandwidthUsage::OVERUSING;
					}
				}
			}
			else if (modifiedTrend < -this->threshold)
			{
				this->timeOverUsingUs.reset();
				this->overuseCounter = 0;
				this->state          = Types::BandwidthUsage::UNDERUSING;
			}
			else
			{
				this->timeOverUsingUs.reset();
				this->overuseCounter = 0;
				this->state          = Types::BandwidthUsage::NORMAL;
			}

			this->prevTrend = trend;

			UpdateThreshold(modifiedTrend, arrivalTimeUs);
		}

		void TrendlineEstimator::UpdateThreshold(double modifiedTrend, int64_t arrivalTimeUs)
		{
			MS_TRACE();

			if (!this->lastThresholdUpdateAtUs.has_value())
			{
				this->lastThresholdUpdateAtUs = arrivalTimeUs;
			}

			// Avoid adapting the threshold to big latency spikes, caused for instance
			// by a sudden capacity drop.
			if (std::fabs(modifiedTrend) > this->threshold + MaxAdaptOffset)
			{
				this->lastThresholdUpdateAtUs = arrivalTimeUs;

				return;
			}

			// The threshold falls faster than it rises, so that it recovers quickly
			// once the network calms down.
			const double coef =
			  std::fabs(modifiedTrend) < this->threshold ? ThresholdDownCoef : ThresholdUpCoef;
			// NOTE: The coefficients above are rates per millisecond, so the step is
			// expressed in those units no matter that the instants are microseconds.
			const double elapsedMs = std::min(
			  static_cast<double>(arrivalTimeUs - this->lastThresholdUpdateAtUs.value()) / 1000.0,
			  static_cast<double>(MaxThresholdUpdateDeltaMs));

			this->threshold += coef * (std::fabs(modifiedTrend) - this->threshold) * elapsedMs;
			this->threshold = std::clamp(this->threshold, ThresholdMin, ThresholdMax);

			this->lastThresholdUpdateAtUs = arrivalTimeUs;
		}
	} // namespace BWE
} // namespace RTC
