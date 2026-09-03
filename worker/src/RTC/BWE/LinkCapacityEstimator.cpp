#define MS_CLASS "RTC::BWE::LinkCapacityEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/LinkCapacityEstimator.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Weight given to a sample taken when overuse was detected. It's low because
		// such a sample only tells that the capacity was exceeded, not where it is.
		static constexpr double OveruseAlpha{ 0.05 };
		// Weight given to a sample measured by a probe, which is far higher since a
		// probe measures the capacity of the link rather than inferring it.
		static constexpr double ProbeAlpha{ 0.5 };
		// Number of standard deviations covered by the confidence interval.
		static constexpr double BoundDeviations{ 3.0 };
		// Bounds of the tracked variance, which is normalized by the estimate itself.
		// The lower one is around 14 kbps at 500 kbps and the upper one around
		// 35 kbps at 500 kbps.
		static constexpr double MinDeviationKbps{ 0.4 };
		static constexpr double MaxDeviationKbps{ 2.5 };

		/* Instance methods. */

		void LinkCapacityEstimator::OnOveruseDetected(int64_t ackedBitrate)
		{
			MS_TRACE();

			Update(ackedBitrate, OveruseAlpha);
		}

		void LinkCapacityEstimator::OnProbeRate(int64_t probeBitrate)
		{
			MS_TRACE();

			Update(probeBitrate, ProbeAlpha);
		}

		std::optional<int64_t> LinkCapacityEstimator::GetEstimate() const
		{
			MS_TRACE();

			if (!this->estimateKbps.has_value())
			{
				return std::nullopt;
			}

			return static_cast<int64_t>(this->estimateKbps.value() * 1000);
		}

		int64_t LinkCapacityEstimator::GetUpperBound() const
		{
			MS_TRACE();

			if (!this->estimateKbps.has_value())
			{
				return Types::BitrateInfinite;
			}

			const double upperBoundKbps =
			  this->estimateKbps.value() + (BoundDeviations * GetDeviationEstimateKbps());

			return static_cast<int64_t>(upperBoundKbps * 1000);
		}

		int64_t LinkCapacityEstimator::GetLowerBound() const
		{
			MS_TRACE();

			if (!this->estimateKbps.has_value())
			{
				return 0;
			}

			const double lowerBoundKbps =
			  std::max(0.0, this->estimateKbps.value() - (BoundDeviations * GetDeviationEstimateKbps()));

			return static_cast<int64_t>(lowerBoundKbps * 1000);
		}

		void LinkCapacityEstimator::Reset()
		{
			MS_TRACE();

			// NOTE: The tracked variance is deliberately kept, since how noisy the
			// samples are is a property of the network rather than of the estimate
			// being dropped.
			this->estimateKbps.reset();
		}

		void LinkCapacityEstimator::Update(int64_t bitrate, double alpha)
		{
			MS_TRACE();

			// NOTE: Samples are taken with a resolution of 1 kbps.
			const double sampleKbps = std::round(static_cast<double>(bitrate) / 1000.0);

			if (!this->estimateKbps.has_value())
			{
				this->estimateKbps = sampleKbps;
			}
			else
			{
				this->estimateKbps = ((1 - alpha) * this->estimateKbps.value()) + (alpha * sampleKbps);
			}

			// Estimate the variance of the estimate, normalized by the estimate itself
			// so that it's comparable across bitrates.
			const double norm      = std::max(this->estimateKbps.value(), 1.0);
			const double errorKbps = this->estimateKbps.value() - sampleKbps;

			this->deviationKbps =
			  ((1 - alpha) * this->deviationKbps) + (alpha * errorKbps * errorKbps / norm);
			this->deviationKbps = std::clamp(this->deviationKbps, MinDeviationKbps, MaxDeviationKbps);
		}

		double LinkCapacityEstimator::GetDeviationEstimateKbps() const
		{
			MS_TRACE();

			// The tracked variance is normalized by the estimate, so it has to be
			// scaled back by it before taking the standard deviation.
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			return std::sqrt(this->deviationKbps * this->estimateKbps.value());
		}
	} // namespace BWE
} // namespace RTC
