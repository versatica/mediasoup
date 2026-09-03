#ifndef MS_RTC_BWE_LINK_CAPACITY_ESTIMATOR_HPP
#define MS_RTC_BWE_LINK_CAPACITY_ESTIMATOR_HPP

#include "common.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Estimates the capacity of the link out of the bitrates at which
		 * congestion was observed.
		 *
		 * Every sample is fed into an exponential filter, and the variance of the
		 * estimate is tracked along with it so that a confidence interval can be
		 * derived. The rate control uses that interval to tell whether the measured
		 * throughput still has anything to do with the capacity it had estimated: a
		 * throughput outside of it means that the link changed, so the estimate is
		 * dropped instead of being slowly filtered towards the new reality.
		 */
		class LinkCapacityEstimator
		{
		public:
			/**
			 * Feed the bitrate at which the network started to be overused, which is
			 * an observation of where the capacity of the link is.
			 *
			 * @param ackedBitrate - Acknowledged bitrate when overuse was detected
			 *   (bps).
			 */
			void OnOveruseDetected(int64_t ackedBitrate);

			/**
			 * Feed the bitrate measured by a probe. It's given far more weight than an
			 * overuse, since a probe measures the capacity of the link directly rather
			 * than inferring it from the delay.
			 *
			 * @param probeBitrate - Bitrate measured by the probe (bps).
			 */
			void OnProbeRate(int64_t probeBitrate);

			/**
			 * Current estimate of the capacity of the link (bps), or no value if
			 * nothing has been observed yet.
			 */
			std::optional<int64_t> GetEstimate() const;

			/**
			 * Upper end of the confidence interval of the estimate (bps), or
			 * `Types::BitrateInfinite` if there is no estimate yet.
			 */
			int64_t GetUpperBound() const;

			/**
			 * Lower end of the confidence interval of the estimate (bps), or 0 if
			 * there is no estimate yet.
			 */
			int64_t GetLowerBound() const;

			/**
			 * Drop the estimate, so that the next sample is taken as is instead of
			 * being filtered against a value that no longer holds.
			 */
			void Reset();

		private:
			void Update(int64_t bitrate, double alpha);

			/**
			 * Standard deviation of the estimate, derived from the normalized variance
			 * being tracked. Only meaningful once there is an estimate.
			 */
			double GetDeviationEstimateKbps() const;

		private:
			// NOTE: The filter works in kbps as double since the bounds applied to the
			// variance are calibrated in those units.
			std::optional<double> estimateKbps;
			double deviationKbps{ 0.4 };
		};
	} // namespace BWE
} // namespace RTC

#endif
