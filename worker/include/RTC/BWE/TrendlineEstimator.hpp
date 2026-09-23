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
		public:
			/**
			 * @remarks
			 * - Every constraint documented below is checked by the constructor and
			 *   aborts when it doesn't hold. These options are set from C++ alone,
			 *   never from the network nor from the API, so breaking one of them is a
			 *   programming error.
			 */
			struct TrendlineEstimatorOptions
			{
				/**
				 * Number of samples of the least squares regression window. A shorter
				 * window reacts sooner at the cost of being noisier.
				 *
				 * @remarks
				 * - It must be at least 2, since a line cannot be fitted to a single
				 *   point.
				 */
				size_t windowSize{ 20 };
				/**
				 * Whether the samples of the window are kept sorted by arrival time.
				 * Feedback should already give them in order, so this is a safety net
				 * against a source that doesn't.
				 */
				bool enableSort{ false };
				/**
				 * Whether the slope is capped by the one that the least delayed packets
				 * of both ends of the window describe, which keeps a burst of delayed
				 * packets in the middle from being read as a growing queue.
				 */
				bool enableCap{ false };
				/**
				 * How many packets of the beginning of the window that cap looks at.
				 *
				 * @remarks
				 * - While `enableCap` is set it must be at least 1, and the two ends
				 *   together must fit in `windowSize`, since the cap looks at both of
				 *   them without them overlapping.
				 */
				size_t beginningPackets{ 7 };
				/**
				 * How many packets of the end of the window that cap looks at.
				 *
				 * @remarks
				 * - The same constraints as `beginningPackets`.
				 */
				size_t endPackets{ 7 };
				/**
				 * Slack added to that cap, so that a slope barely above it is not
				 * capped.
				 */
				double capUncertainty{ 0.0 };
			};

		private:
			struct PacketTiming
			{
				double arrivalTimeUs;
				double smoothedDelayUs;
				/**
				 * Accumulated delay before it's smoothed, which is what the slope cap
				 * looks at.
				 */
				double rawDelayUs;
			};

		public:
			TrendlineEstimator();

			explicit TrendlineEstimator(TrendlineEstimatorOptions options);

			/**
			 * Feed the deltas between two consecutive groups of packets.
			 *
			 * @param sendDeltaUs - Time elapsed between the send times of both groups.
			 * @param arrivalDeltaUs - Time elapsed between the arrival times of both
			 *   groups.
			 * @param arrivalTimeUs - Arrival time of the latest group, in the remote
			 *   clock reference.
			 */
			void Update(int64_t sendDeltaUs, int64_t arrivalDeltaUs, int64_t arrivalTimeUs);

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
			std::optional<double> GetLinearFitSlope() const;

			/**
			 * Highest slope that the least delayed samples of both ends of the window
			 * describe, or no value if they are too close in time to tell.
			 *
			 * @remarks
			 * - A burst of delayed packets in the middle of the window tilts the
			 *   fitted line even though the queue never grew, and this is what keeps
			 *   that from being read as overuse.
			 */
			std::optional<double> GetSlopeCap() const;

			void Detect(double trend, double sendDeltaUs, int64_t arrivalTimeUs);

			void UpdateThreshold(double modifiedTrend, int64_t arrivalTimeUs);

		private:
			const TrendlineEstimatorOptions options;
			int numOfDeltas{ 0 };
			std::optional<int64_t> firstArrivalTimeUs;
			double accumulatedDelayUs{ 0 };
			double smoothedDelayUs{ 0 };
			std::deque<PacketTiming> delayHist;
			double threshold{ 12.5 };
			double prevTrend{ 0 };
			std::optional<double> timeOverUsingUs;
			int overuseCounter{ 0 };
			std::optional<int64_t> lastThresholdUpdateAtUs;
			Types::BandwidthUsage state{ Types::BandwidthUsage::NORMAL };
		};
	} // namespace BWE
} // namespace RTC

#endif
