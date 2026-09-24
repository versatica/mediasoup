#ifndef MS_RTC_BWE_OVERUSE_ESTIMATOR_HPP
#define MS_RTC_BWE_OVERUSE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <deque>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Estimates how much the delay between groups of packets is growing, and how
		 * noisy that measure is.
		 *
		 * It fits a two variable model to what each pair of groups showed: how much
		 * of the extra delay is explained by the group being bigger, and how much is
		 * left over once that is taken out. The second one is the interesting part,
		 * since a delay that grows without the traffic growing is a queue filling up
		 * somewhere.
		 *
		 * Both are tracked with a Kalman filter, which is what lets the estimate
		 * follow the network while telling apart a real trend from the jitter that
		 * every link has. The noise it measures along the way is what the detector
		 * compares the trend against.
		 *
		 * @remarks
		 * - This is what the receiving side uses, where the only thing known about
		 *   when a packet was sent is the timestamp it carries. The sending side has
		 *   the real instants, so it uses `TrendlineEstimator` instead, which fits a
		 *   line rather than filtering.
		 */
		class OveruseEstimator
		{
		public:
			OveruseEstimator() = default;

			~OveruseEstimator() = default;

			OveruseEstimator(const OveruseEstimator&)            = delete;
			OveruseEstimator& operator=(const OveruseEstimator&) = delete;

			/**
			 * Feed the deltas between two consecutive groups of packets.
			 *
			 * @param arrivalDeltaMs - Time elapsed between the arrival of both groups.
			 * @param sendDeltaMs - Time elapsed between the send times of both groups.
			 * @param sizeDelta - Difference of size between both groups (bytes).
			 * @param currentHypothesis - What the detector makes of the network right
			 *   now, which decides whether this sample is used to measure the noise
			 *   and how much the filter is allowed to move.
			 */
			void Update(
			  double arrivalDeltaMs,
			  double sendDeltaMs,
			  int64_t sizeDelta,
			  Types::BandwidthUsage currentHypothesis);

			/**
			 * Estimated variance of the noise (ms^2).
			 */
			double GetVarNoiseMs2() const
			{
				return this->varNoiseMs2;
			}

			/**
			 * Estimated part of the delay between groups that the traffic itself
			 * doesn't explain (ms).
			 */
			double GetOffsetMs() const
			{
				return this->offsetMs;
			}

			/**
			 * Number of samples the current estimate is based on.
			 */
			int64_t GetNumOfDeltas() const
			{
				return this->numOfDeltas;
			}

		private:
			/**
			 * Shortest send delta of the recent history, which is what the noise
			 * estimate is scaled by so that it doesn't depend on how often packets
			 * come.
			 */
			double UpdateMinFramePeriod(double sendDeltaMs);

			void UpdateNoiseEstimate(double residual, double sendDeltaMs, bool stableState);

		private:
			int64_t numOfDeltas{ 0 };
			// How much of the extra delay a bigger group explains (ms per byte).
			double slopeMsPerByte{ 8.0 / 512.0 };
			// What is left of it once that is taken out (ms).
			double offsetMs{ 0.0 };
			double prevOffsetMs{ 0.0 };
			// Covariance of the two values above, and how much they are allowed to
			// move on their own between samples.
			double e[2][2]{
				{ 100.0, 0.0  },
        { 0.0,   1e-1 }
			};
			double processNoise[2]{ 1e-13, 1e-3 };
			double avgNoiseMs{ 0.0 };
			double varNoiseMs2{ 50.0 };
			std::deque<double> sendDeltaHistMs;
		};
	} // namespace BWE
} // namespace RTC

#endif
