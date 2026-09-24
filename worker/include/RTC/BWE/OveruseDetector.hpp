#ifndef MS_RTC_BWE_OVERUSE_DETECTOR_HPP
#define MS_RTC_BWE_OVERUSE_DETECTOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Decides whether the network is congested from how much the delay between
		 * groups of packets is growing.
		 *
		 * The trend it is given is compared against a threshold that adapts itself
		 * to the noise observed, and congestion is only declared once the condition
		 * has persisted, which is what keeps an isolated jitter spike from bringing
		 * the estimate down.
		 *
		 * @remarks
		 * - This is what the receiving side uses, paired with `OveruseEstimator`.
		 *   The sending side has `TrendlineEstimator`, which does both jobs at once
		 *   and shares this very threshold machinery.
		 */
		class OveruseDetector
		{
		public:
			OveruseDetector() = default;

			~OveruseDetector() = default;

			OveruseDetector(const OveruseDetector&)            = delete;
			OveruseDetector& operator=(const OveruseDetector&) = delete;

			/**
			 * Feed what the estimator makes of the latest pair of groups.
			 *
			 * @param offsetMs - Part of the delay between both groups that the traffic
			 *   itself doesn't explain.
			 * @param sendDeltaMs - Time between the send times of both groups, which
			 *   is how long this sample is worth.
			 * @param numOfDeltas - Number of samples that offset is based on.
			 * @param nowUs - Current time.
			 *
			 * @returns What the network is taken to be doing after this sample.
			 */
			Types::BandwidthUsage Detect(
			  double offsetMs, double sendDeltaMs, int64_t numOfDeltas, int64_t nowUs);

			Types::BandwidthUsage GetState() const
			{
				return this->state;
			}

		private:
			void UpdateThreshold(double modifiedOffsetMs, int64_t nowUs);

		private:
			double threshold{ 12.5 };
			std::optional<int64_t> lastThresholdUpdateAtUs;
			double prevOffsetMs{ 0.0 };
			// How long the trend has been above the threshold, or no value while it
			// isn't.
			std::optional<double> timeOverUsingMs;
			int64_t overuseCounter{ 0 };
			Types::BandwidthUsage state{ Types::BandwidthUsage::NORMAL };
		};
	} // namespace BWE
} // namespace RTC

#endif
