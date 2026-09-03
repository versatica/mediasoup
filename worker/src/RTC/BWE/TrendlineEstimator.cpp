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
		// Rates at which the threshold adapts upwards and downwards.
		static constexpr double ThresholdUpCoef{ 0.0087 };
		static constexpr double ThresholdDownCoef{ 0.039 };
		// Bounds of the adaptive threshold.
		static constexpr double ThresholdMin{ 6.0 };
		static constexpr double ThresholdMax{ 600.0 };
		// Distance from the threshold beyond which it stops adapting, so that a
		// sudden capacity drop doesn't drag it along.
		static constexpr double MaxAdaptOffsetMs{ 15.0 };
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

			// NOTE: The trendline math is done in ms as double since every constant
			// above is expressed in those units. Being fed us means that the
			// sub-millisecond precision survives as the fractional part.
			const double sendDeltaMs = static_cast<double>(sendDeltaUs) / 1000.0;
			const double deltaMs     = static_cast<double>(arrivalDeltaUs - sendDeltaUs) / 1000.0;

			this->numOfDeltas = std::min(this->numOfDeltas + 1, DeltaCounterMax);

			if (!this->firstArrivalTimeUs.has_value())
			{
				this->firstArrivalTimeUs = arrivalTimeUs;
			}

			// Exponential backoff filter.
			this->accumulatedDelayMs += deltaMs;
			this->smoothedDelayMs =
			  (SmoothingCoef * this->smoothedDelayMs) + ((1 - SmoothingCoef) * this->accumulatedDelayMs);

			// Maintain the samples window. A group may arrive before the first one of
			// the window did, in which case the regression just gets a negative x.
			const double elapsedMs =
			  static_cast<double>(arrivalTimeUs - this->firstArrivalTimeUs.value()) / 1000.0;

			this->delayHist.push_back({ elapsedMs, this->smoothedDelayMs });

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
				trend = LinearFitSlope().value_or(trend);
			}

			Detect(trend, sendDeltaMs, arrivalTimeUs / 1000);
		}

		std::optional<double> TrendlineEstimator::LinearFitSlope() const
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
				sumX += sample.arrivalTimeMs;
				sumY += sample.smoothedDelayMs;
			}

			const double avgX = sumX / this->delayHist.size();
			const double avgY = sumY / this->delayHist.size();

			// Compute the slope as sum((x - avgX) * (y - avgY)) / sum((x - avgX)^2).
			double numerator{ 0 };
			double denominator{ 0 };

			for (const auto& sample : this->delayHist)
			{
				const double x = sample.arrivalTimeMs - avgX;
				const double y = sample.smoothedDelayMs - avgY;

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

		void TrendlineEstimator::Detect(double trend, double sendDeltaMs, int64_t arrivalTimeMs)
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
				if (this->timeOverUsingMs.value() > OverusingTimeThresholdMs && this->overuseCounter > 1)
				{
					if (trend >= this->prevTrend)
					{
						this->timeOverUsingMs = 0;
						this->overuseCounter  = 0;
						this->state           = Types::BandwidthUsage::OVERUSING;
					}
				}
			}
			else if (modifiedTrend < -this->threshold)
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

			this->prevTrend = trend;

			UpdateThreshold(modifiedTrend, arrivalTimeMs);
		}

		void TrendlineEstimator::UpdateThreshold(double modifiedTrend, int64_t arrivalTimeMs)
		{
			MS_TRACE();

			if (!this->lastThresholdUpdateTimeMs.has_value())
			{
				this->lastThresholdUpdateTimeMs = arrivalTimeMs;
			}

			// Avoid adapting the threshold to big latency spikes, caused for instance
			// by a sudden capacity drop.
			if (std::fabs(modifiedTrend) > this->threshold + MaxAdaptOffsetMs)
			{
				this->lastThresholdUpdateTimeMs = arrivalTimeMs;

				return;
			}

			// The threshold falls faster than it rises, so that it recovers quickly
			// once the network calms down.
			const double coef =
			  std::fabs(modifiedTrend) < this->threshold ? ThresholdDownCoef : ThresholdUpCoef;
			const int64_t elapsedMs =
			  std::min(arrivalTimeMs - this->lastThresholdUpdateTimeMs.value(), MaxThresholdUpdateDeltaMs);

			this->threshold += coef * (std::fabs(modifiedTrend) - this->threshold) * elapsedMs;
			this->threshold = std::clamp(this->threshold, ThresholdMin, ThresholdMax);

			this->lastThresholdUpdateTimeMs = arrivalTimeMs;
		}
	} // namespace BWE
} // namespace RTC
