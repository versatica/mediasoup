#ifndef MS_RTC_BWE_TRENDLINE_ESTIMATOR_HPP
#define MS_RTC_BWE_TRENDLINE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <deque>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Detects network congestion from the delay between groups of packets,
		 * before any packet is lost.
		 *
		 * For each pair of consecutive groups it accumulates `arrivalDelta -
		 * sendDelta`, smooths it, and fits a least squares line over the last
		 * `WindowSize` samples. The slope of that line approximates
		 * `(sendRate - capacity) / capacity`, so a positive slope means that queues
		 * are filling up somewhere in the network.
		 *
		 * That slope is compared against a threshold that adapts itself to the
		 * observed noise, and overuse is only declared once the condition persists,
		 * which is what prevents an isolated jitter spike from bringing the estimate
		 * down.
		 */
		class TrendlineEstimator
		{
		private:
			struct PacketTiming
			{
				double arrivalTimeMs;
				double smoothedDelayMs;
			};

		public:
			/**
			 * Number of samples of the least squares regression window used unless
			 * another one is given.
			 */
			static constexpr size_t DefaultWindowSize{ 20 };

		public:
			/**
			 * @param windowSize - Number of samples of the least squares regression
			 *   window. A shorter window reacts sooner at the cost of being noisier.
			 */
			explicit TrendlineEstimator(size_t windowSize = DefaultWindowSize);

			/**
			 * Feed the deltas between two consecutive groups of packets.
			 *
			 * @param sendDeltaUs - Time elapsed between the send times of both groups.
			 * @param arrivalDeltaUs - Time elapsed between the arrival times of both
			 *   groups.
			 * @param arrivalTimeUs - Arrival time of the latest group, in the remote
			 *   clock reference.
			 */
			void Update(int64_t sendDeltaUs, int64_t arrivalDeltaUs, uint64_t arrivalTimeUs);

			/**
			 * Current hypothesis about how the network is behaving.
			 */
			Types::BandwidthUsage GetState() const
			{
				return this->state;
			}

		private:
			/**
			 * Slope of the least squares line fitted to the samples window, or no
			 * value if it cannot be computed.
			 */
			std::optional<double> LinearFitSlope() const;

			void Detect(double trend, double sendDeltaMs, int64_t arrivalTimeMs);

			void UpdateThreshold(double modifiedTrend, int64_t arrivalTimeMs);

		private:
			const size_t windowSize;
			int numOfDeltas{ 0 };
			std::optional<uint64_t> firstArrivalTimeUs;
			double accumulatedDelayMs{ 0 };
			double smoothedDelayMs{ 0 };
			std::deque<PacketTiming> delayHist;
			double threshold{ 12.5 };
			double prevTrend{ 0 };
			std::optional<double> timeOverUsingMs;
			int overuseCounter{ 0 };
			std::optional<int64_t> lastThresholdUpdateTimeMs;
			Types::BandwidthUsage state{ Types::BandwidthUsage::NORMAL };
		};
	} // namespace BWE
} // namespace RTC

#endif
