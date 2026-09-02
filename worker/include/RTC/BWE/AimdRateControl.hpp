#ifndef MS_RTC_BWE_AIMD_RATE_CONTROL_HPP
#define MS_RTC_BWE_AIMD_RATE_CONTROL_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/LinkCapacityEstimator.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Decides the target bitrate out of what the delay based detector says about
		 * the network.
		 *
		 * While no overuse is detected the bitrate is increased, multiplicatively
		 * when the capacity of the link is still unknown and additively once it has
		 * been estimated, since being close to the capacity makes a big step
		 * expensive. On overuse it is decreased to slightly below the measured
		 * throughput and held there until the queues of the network drain.
		 */
		class AimdRateControl
		{
		public:
			struct AimdRateControlOptions
			{
				/**
				 * Fraction of the measured throughput the bitrate is dropped to when
				 * overuse is detected.
				 */
				double backoffFactor{ 0.85 };
				/**
				 * Whether the bitrate is not allowed to increase while the sender is in
				 * an application limited region. In that region the network gives no
				 * feedback about whether a higher bitrate would have worked, so an
				 * increase cannot be validated.
				 */
				bool noBitrateIncreaseInAlr{ false };
				/**
				 * Whether the estimate of the link capacity is used as an upper bound.
				 */
				bool estimateBoundedIncrease{ true };
				/**
				 * Whether the current bitrate is taken as the lowest possible upper
				 * bound, so that an estimate of the link capacity below it does not
				 * drag the bitrate down.
				 */
				bool useCurrentEstimateAsMinUpperBound{ true };
				/**
				 * Whether this rate control runs in the endpoint that sends the media
				 * whose capacity is being estimated, rather than in the one that
				 * receives it. Only the sender can know that it's in an application
				 * limited region, so `noBitrateIncreaseInAlr` has no effect otherwise.
				 */
				bool sendSide{ false };
			};

		private:
			enum class RateControlState : uint8_t
			{
				HOLD,
				INCREASE,
				DECREASE
			};

		public:
			AimdRateControl();

			explicit AimdRateControl(AimdRateControlOptions options);

			/**
			 * Whether the target bitrate has been initialized, which happens either
			 * when it's explicitly set or once a throughput has been measured for long
			 * enough.
			 */
			bool ValidEstimate() const
			{
				return this->bitrateIsInitialized;
			}

			/**
			 * Current target bitrate (bps).
			 */
			int64_t LatestEstimate() const
			{
				return this->currentBitrate;
			}

			/**
			 * Feed what the delay based detector says about the network.
			 *
			 * @param input - State of the network plus the acknowledged bitrate.
			 * @param atTimeUs - Time at which the input was produced.
			 *
			 * @returns The new target bitrate (bps).
			 */
			int64_t Update(const Types::RateControlInput& input, int64_t atTimeUs);

			/**
			 * Set the target bitrate from outside the rate control, for instance out of
			 * the bitrate measured by a probe.
			 *
			 * @param bitrate - New target bitrate (bps).
			 */
			void SetEstimate(int64_t bitrate, int64_t atTimeUs);

			/**
			 * @param startBitrate - Bitrate to start from (bps).
			 */
			void SetStartBitrate(int64_t startBitrate);

			/**
			 * @param minBitrate - Bitrate the target is never taken below (bps).
			 */
			void SetMinBitrate(int64_t minBitrate);

			void SetRtt(int64_t rttUs);

			void SetInApplicationLimitedRegion(bool inAlr);

			void SetNetworkStateEstimate(const std::optional<Types::NetworkStateEstimate>& estimate);

			/**
			 * Whether the bitrate can be reduced again, which requires that it has not
			 * been changed for more than an RTT or that the throughput has fallen well
			 * below the current target. It's what keeps a single overuse from
			 * collapsing the bitrate in a few milliseconds.
			 */
			bool TimeToReduceFurther(int64_t atTimeUs, int64_t estimatedThroughput) const;

			/**
			 * As above, to be used when overusing before any throughput has been
			 * measured.
			 */
			bool InitialTimeToReduceFurther(int64_t atTimeUs) const;

			/**
			 * Rate at which the bitrate is increased once it's close to the capacity of
			 * the link (bps per second).
			 */
			double GetNearMaxIncreaseRateBpsPerSecond() const;

			/**
			 * How often feedback can be sent while devoting no more than 5% of the
			 * current bitrate to it.
			 */
			int64_t GetFeedbackIntervalUs() const;

		private:
			void ChangeBitrate(const Types::RateControlInput& input, int64_t atTimeUs);

			void ChangeState(const Types::RateControlInput& input, int64_t atTimeUs);

			/**
			 * Apply the configured bounds to a candidate bitrate.
			 */
			int64_t ClampBitrate(int64_t newBitrate) const;

			/**
			 * Increase used while the capacity of the link is unknown, which ramps up
			 * fast to discover it.
			 */
			int64_t MultiplicativeRateIncrease(
			  int64_t atTimeUs, std::optional<int64_t> lastTimeUs, int64_t currentBitrate) const;

			/**
			 * Increase used once the capacity of the link has been estimated, which
			 * approaches it in small steps.
			 */
			int64_t AdditiveRateIncrease(int64_t atTimeUs, int64_t lastTimeUs) const;

		private:
			const AimdRateControlOptions options;
			int64_t minConfiguredBitrate;
			int64_t maxConfiguredBitrate;
			int64_t currentBitrate;
			int64_t latestEstimatedThroughput;
			LinkCapacityEstimator linkCapacity;
			std::optional<Types::NetworkStateEstimate> networkEstimate;
			RateControlState rateControlState{ RateControlState::HOLD };
			std::optional<int64_t> timeLastBitrateChangeUs;
			std::optional<int64_t> timeFirstThroughputEstimateUs;
			bool bitrateIsInitialized{ false };
			bool inAlr{ false };
			int64_t rttUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
