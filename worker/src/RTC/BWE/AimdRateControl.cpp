#define MS_CLASS "RTC::BWE::AimdRateControl"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/AimdRateControl.hpp"
#include "Logger.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// RTT assumed until a measured one is given.
		static constexpr int64_t DefaultRttUs{ 200 * 1000 };
		// Time a throughput has to be measured for before it's taken as the initial
		// target bitrate.
		static constexpr int64_t InitializationTimeUs{ 5 * 1000 * 1000 };
		// Window over which the throughput is measured.
		static constexpr int64_t BitrateWindowUs{ 1000 * 1000 };
		// Bounds of how often the bitrate may be reduced, derived from the RTT.
		static constexpr int64_t MinBitrateReductionIntervalUs{ 10 * 1000 };
		static constexpr int64_t MaxBitrateReductionIntervalUs{ 200 * 1000 };
		// Bitrate the target is never taken below unless configured otherwise (bps).
		static constexpr int64_t CongestionControllerMinBitrate{ 5000 };
		// Bitrate the target starts from before anything has been measured (bps).
		static constexpr int64_t MaxConfiguredBitrate{ 30000 * 1000 };
		// Headroom allowed on top of the measured throughput when increasing (bps).
		static constexpr int64_t IncreaseLimitHeadroom{ 10 * 1000 };
		// Amount subtracted from the backed off bitrate to get rid of any
		// self-induced delay (bps).
		static constexpr int64_t DecreaseMargin{ 5 * 1000 };
		// Smallest multiplicative increase applied (bps).
		static constexpr int64_t MinMultiplicativeIncrease{ 1000 };
		// Base of the multiplicative increase, which is 8% per second.
		static constexpr double MultiplicativeIncreaseAlpha{ 1.08 };
		// Interval between frames assumed when sizing the additive increase.
		static constexpr int64_t FrameIntervalUs{ 1000 * 1000 / 30 };
		// Packet size assumed when sizing the additive increase (bytes).
		static constexpr int64_t PacketSizeBytes{ 1200 };
		// Delay of the overuse detector assumed when sizing the additive increase.
		static constexpr int64_t DetectorDelayUs{ 100 * 1000 };
		// Smallest additive increase applied (bps per second).
		static constexpr double MinIncreaseRateBpsPerSecond{ 4000 };
		// Size of a feedback message assumed when deciding how often to send it
		// (bytes).
		static constexpr int64_t RtcpSizeBytes{ 80 };
		// Fraction of the bitrate devoted to feedback.
		static constexpr double FeedbackBandwidthFraction{ 0.05 };
		// Bounds of how often feedback is sent.
		static constexpr int64_t MinFeedbackIntervalUs{ 200 * 1000 };
		static constexpr int64_t MaxFeedbackIntervalUs{ 1000 * 1000 };

		static_assert(
		  BitrateWindowUs <= InitializationTimeUs,
		  "the throughput window must fit within the initialization time");

		/* Instance methods. */

		AimdRateControl::AimdRateControl() : AimdRateControl(AimdRateControlOptions{})
		{
			MS_TRACE();
		}

		AimdRateControl::AimdRateControl(AimdRateControlOptions options)
		  : options(options),
		    minConfiguredBitrate(CongestionControllerMinBitrate),
		    maxConfiguredBitrate(MaxConfiguredBitrate),
		    currentBitrate(this->maxConfiguredBitrate),
		    latestEstimatedThroughput(this->currentBitrate),
		    rttUs(DefaultRttUs)
		{
			MS_TRACE();
		}

		int64_t AimdRateControl::Update(const Types::RateControlInput& input, int64_t atTimeUs)
		{
			MS_TRACE();

			// Take the throughput being measured as the initial target bitrate, once it
			// has been measured for long enough.
			if (!this->bitrateIsInitialized)
			{
				if (!this->timeFirstThroughputEstimateUs.has_value())
				{
					if (input.estimatedThroughput.has_value())
					{
						this->timeFirstThroughputEstimateUs = atTimeUs;
					}
				}
				else if (
				  atTimeUs - this->timeFirstThroughputEstimateUs.value() > InitializationTimeUs &&
				  input.estimatedThroughput.has_value())
				{
					this->currentBitrate       = input.estimatedThroughput.value();
					this->bitrateIsInitialized = true;
				}
			}

			ChangeBitrate(input, atTimeUs);

			return this->currentBitrate;
		}

		void AimdRateControl::SetEstimate(int64_t bitrate, int64_t atTimeUs)
		{
			MS_TRACE();

			this->bitrateIsInitialized    = true;
			this->currentBitrate          = ClampBitrate(bitrate);
			this->timeLastBitrateChangeUs = atTimeUs;
		}

		void AimdRateControl::SetStartBitrate(int64_t startBitrate)
		{
			MS_TRACE();

			this->currentBitrate            = startBitrate;
			this->latestEstimatedThroughput = this->currentBitrate;
			this->bitrateIsInitialized      = true;
		}

		void AimdRateControl::SetMinBitrate(int64_t minBitrate)
		{
			MS_TRACE();

			this->minConfiguredBitrate = minBitrate;
			this->currentBitrate       = std::max(minBitrate, this->currentBitrate);
		}

		void AimdRateControl::SetRtt(int64_t rttUs)
		{
			MS_TRACE();

			this->rttUs = rttUs;
		}

		void AimdRateControl::SetInApplicationLimitedRegion(bool inAlr)
		{
			MS_TRACE();

			this->inAlr = inAlr;
		}

		void AimdRateControl::SetNetworkStateEstimate(
		  const std::optional<Types::NetworkStateEstimate>& estimate)
		{
			MS_TRACE();

			this->networkEstimate = estimate;
		}

		bool AimdRateControl::TimeToReduceFurther(int64_t atTimeUs, int64_t estimatedThroughput) const
		{
			MS_TRACE();

			const int64_t bitrateReductionIntervalUs =
			  std::clamp(this->rttUs, MinBitrateReductionIntervalUs, MaxBitrateReductionIntervalUs);

			// The bitrate has never been changed, so there is nothing to wait for.
			if (!this->timeLastBitrateChangeUs.has_value())
			{
				return true;
			}

			if (atTimeUs - this->timeLastBitrateChangeUs.value() >= bitrateReductionIntervalUs)
			{
				return true;
			}

			if (ValidEstimate())
			{
				const int64_t threshold = std::llround(0.5 * static_cast<double>(LatestEstimate()));

				return estimatedThroughput < threshold;
			}

			return false;
		}

		bool AimdRateControl::InitialTimeToReduceFurther(int64_t atTimeUs) const
		{
			MS_TRACE();

			return ValidEstimate() && TimeToReduceFurther(atTimeUs, (LatestEstimate() / 2) - 1);
		}

		double AimdRateControl::GetNearMaxIncreaseRateBpsPerSecond() const
		{
			MS_TRACE();

			MS_ASSERT(this->currentBitrate != 0, "current bitrate is zero");

			// Size of a frame at the current bitrate, rounded to the nearest byte.
			const int64_t frameSizeBytes = ((this->currentBitrate * FrameIntervalUs) + 4000000) / 8000000;
			const double packetsPerFrame =
			  std::ceil(static_cast<double>(frameSizeBytes) / static_cast<double>(PacketSizeBytes));
			const int64_t avgPacketSizeBytes =
			  std::llround(static_cast<double>(frameSizeBytes) / packetsPerFrame);

			// Time the network takes to react to a change, which is a round trip plus
			// the delay of the overuse detector, doubled to stay on the safe side.
			const int64_t responseTimeUs = (this->rttUs + DetectorDelayUs) * 2;
			const double increaseRateBpsPerSecond =
			  static_cast<double>(avgPacketSizeBytes * 8000000 / responseTimeUs);

			return std::max(MinIncreaseRateBpsPerSecond, increaseRateBpsPerSecond);
		}

		int64_t AimdRateControl::GetFeedbackIntervalUs() const
		{
			MS_TRACE();

			const int64_t rtcpBitrate = std::llround(FeedbackBandwidthFraction * this->currentBitrate);
			const int64_t intervalUs  = (RtcpSizeBytes * 8 * 1000000) / rtcpBitrate;

			return std::clamp(intervalUs, MinFeedbackIntervalUs, MaxFeedbackIntervalUs);
		}

		void AimdRateControl::ChangeBitrate(const Types::RateControlInput& input, int64_t atTimeUs)
		{
			MS_TRACE();

			std::optional<int64_t> newBitrate;
			const int64_t estimatedThroughput =
			  input.estimatedThroughput.value_or(this->latestEstimatedThroughput);

			if (input.estimatedThroughput.has_value())
			{
				this->latestEstimatedThroughput = input.estimatedThroughput.value();
			}

			// An overuse must always make us reduce the bitrate, even before the first
			// estimate has been established. By acting on it we end up with a valid
			// estimate.
			if (!this->bitrateIsInitialized && input.bandwidthUsage != Types::BandwidthUsage::OVERUSING)
			{
				return;
			}

			ChangeState(input, atTimeUs);

			switch (this->rateControlState)
			{
				case RateControlState::HOLD:
				{
					break;
				}

				case RateControlState::INCREASE:
				{
					// The measured throughput is way above what the link was estimated to
					// hold, so that estimate no longer describes this link.
					if (estimatedThroughput > this->linkCapacity.GetUpperBound())
					{
						this->linkCapacity.Reset();
					}

					// Limit the new bitrate by the throughput to avoid unbounded increases.
					// A bit more lag is allowed at very low rates so that an encoder with
					// uneven output doesn't get us stuck.
					int64_t increaseLimit =
					  std::llround(1.5 * static_cast<double>(estimatedThroughput)) + IncreaseLimitHeadroom;

					if (this->options.sendSide && this->inAlr && this->options.noBitrateIncreaseInAlr)
					{
						// Don't increase the delay based estimate while in an application
						// limited region, since the network gives no feedback that could tell
						// whether the new estimate is correct. If we had previously increased
						// above the limit, for instance because of a probe, no further change
						// is allowed either.
						increaseLimit = this->currentBitrate;
					}

					if (this->currentBitrate < increaseLimit)
					{
						int64_t increasedBitrate{ 0 };
						const auto linkCapacityEstimate = this->linkCapacity.GetEstimate();

						if (linkCapacityEstimate.has_value())
						{
							// The estimate of the link capacity is dropped whenever the measured
							// throughput strays too far from it, so having one means that the
							// target is reasonably close to the capacity and a small step is
							// enough.
							MS_ASSERT(
							  this->timeLastBitrateChangeUs.has_value(),
							  "there is a link capacity estimate but the bitrate was never changed");

							const int64_t additiveIncrease =
							  AdditiveRateIncrease(atTimeUs, this->timeLastBitrateChangeUs.value());

							increasedBitrate = this->currentBitrate + additiveIncrease;
						}
						else
						{
							// Without an estimate of the link capacity, ramp up faster to
							// discover it.
							const int64_t multiplicativeIncrease = MultiplicativeRateIncrease(
							  atTimeUs, this->timeLastBitrateChangeUs, this->currentBitrate);

							increasedBitrate = this->currentBitrate + multiplicativeIncrease;
						}

						newBitrate = std::min(increasedBitrate, increaseLimit);
					}

					this->timeLastBitrateChangeUs = atTimeUs;

					break;
				}

				case RateControlState::DECREASE:
				{
					// Drop slightly below the measured throughput to get rid of any
					// self-induced delay.
					int64_t decreasedBitrate =
					  std::llround(this->options.backoffFactor * static_cast<double>(estimatedThroughput));

					if (decreasedBitrate > DecreaseMargin)
					{
						decreasedBitrate -= DecreaseMargin;
					}

					if (decreasedBitrate > this->currentBitrate)
					{
						const auto linkCapacityEstimate = this->linkCapacity.GetEstimate();

						if (linkCapacityEstimate.has_value())
						{
							decreasedBitrate = std::llround(
							  this->options.backoffFactor * static_cast<double>(linkCapacityEstimate.value()));
						}
					}

					// Avoid increasing the bitrate while overusing.
					if (decreasedBitrate < this->currentBitrate)
					{
						newBitrate = decreasedBitrate;
					}

					// The throughput is far below what the link was estimated to hold, so
					// drop that estimate to let the overuse below update it right away.
					if (estimatedThroughput < this->linkCapacity.GetLowerBound())
					{
						this->linkCapacity.Reset();
					}

					this->bitrateIsInitialized = true;

					this->linkCapacity.OnOveruseDetected(estimatedThroughput);

					// Stay on hold until the queues of the network are drained.
					this->rateControlState        = RateControlState::HOLD;
					this->timeLastBitrateChangeUs = atTimeUs;

					break;
				}

					NO_DEFAULT_GCC();
			}

			this->currentBitrate = ClampBitrate(newBitrate.value_or(this->currentBitrate));
		}

		void AimdRateControl::ChangeState(const Types::RateControlInput& input, int64_t atTimeUs)
		{
			MS_TRACE();

			switch (input.bandwidthUsage)
			{
				case Types::BandwidthUsage::NORMAL:
				{
					if (this->rateControlState == RateControlState::HOLD)
					{
						this->timeLastBitrateChangeUs = atTimeUs;
						this->rateControlState        = RateControlState::INCREASE;
					}

					break;
				}

				case Types::BandwidthUsage::OVERUSING:
				{
					if (this->rateControlState != RateControlState::DECREASE)
					{
						this->rateControlState = RateControlState::DECREASE;
					}

					break;
				}

				case Types::BandwidthUsage::UNDERUSING:
				{
					this->rateControlState = RateControlState::HOLD;

					break;
				}

					NO_DEFAULT_GCC();
			}
		}

		int64_t AimdRateControl::ClampBitrate(int64_t newBitrate) const
		{
			MS_TRACE();

			if (
			  this->options.estimateBoundedIncrease && this->networkEstimate.has_value() &&
			  this->networkEstimate.value().linkCapacityUpper.has_value())
			{
				const int64_t linkCapacityUpper = this->networkEstimate.value().linkCapacityUpper.value();
				const int64_t upperBound        = this->options.useCurrentEstimateAsMinUpperBound
				                                    ? std::max(linkCapacityUpper, this->currentBitrate)
				                                    : linkCapacityUpper;

				newBitrate = std::min(upperBound, newBitrate);
			}

			if (
			  this->networkEstimate.has_value() &&
			  this->networkEstimate.value().linkCapacityLower.has_value() &&
			  newBitrate < this->currentBitrate)
			{
				const int64_t linkCapacityLower = this->networkEstimate.value().linkCapacityLower.value();

				newBitrate = std::min(
				  this->currentBitrate,
				  std::max(
				    newBitrate,
				    std::llround(this->options.backoffFactor * static_cast<double>(linkCapacityLower))));
			}

			return std::max(newBitrate, this->minConfiguredBitrate);
		}

		int64_t AimdRateControl::MultiplicativeRateIncrease(
		  int64_t atTimeUs, std::optional<int64_t> lastTimeUs, int64_t currentBitrate) const
		{
			MS_TRACE();

			double alpha{ MultiplicativeIncreaseAlpha };

			if (lastTimeUs.has_value())
			{
				const double timeSinceLastUpdateSeconds =
				  static_cast<double>(atTimeUs - lastTimeUs.value()) / 1000000.0;

				alpha = std::pow(alpha, std::min(timeSinceLastUpdateSeconds, 1.0));
			}

			return std::max(
			  std::llround(static_cast<double>(currentBitrate) * (alpha - 1.0)), MinMultiplicativeIncrease);
		}

		int64_t AimdRateControl::AdditiveRateIncrease(int64_t atTimeUs, int64_t lastTimeUs) const
		{
			MS_TRACE();

			const double timePeriodSeconds = static_cast<double>(atTimeUs - lastTimeUs) / 1000000.0;

			return static_cast<int64_t>(GetNearMaxIncreaseRateBpsPerSecond() * timePeriodSeconds);
		}
	} // namespace BWE
} // namespace RTC
