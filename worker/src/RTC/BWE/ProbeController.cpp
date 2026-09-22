#define MS_CLASS "RTC::BWE::ProbeController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ProbeController.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BitrateUtils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How long to wait for the result of a burst before giving up on it.
		static constexpr int64_t MaxWaitingTimeForProbingResultUs{ 1000 * 1000 };
		// Highest bitrate a burst may aim for while the application sets no maximum.
		//
		// NOTE: libwebrtc uses 5 Mbps here, which is a sensible ceiling for a browser
		// sending a couple of streams and a low one for a transport that carries many.
		static constexpr int64_t DefaultMaxProbingBitrate{ 10000000 };
		// Fraction of the previous estimate below which a new one counts as a large
		// drop rather than as the estimate moving.
		static constexpr double BitrateDropThreshold{ 0.66 };
		// How long after a large drop it's still worth probing to find out whether it
		// was real.
		static constexpr int64_t BitrateDropTimeoutUs{ 5 * 1000 * 1000 };
		// Fraction of what the estimate was worth before a large drop that the burst
		// checking it aims for.
		static constexpr double ProbeFractionAfterDrop{ 0.85 };
		// How long after the sender goes back to filling the link a burst is still
		// taken as one sent while it wasn't.
		static constexpr int64_t AlrEndedTimeoutUs{ 3 * 1000 * 1000 };
		// Shortest time between two bursts caused by a large drop.
		static constexpr int64_t MinTimeBetweenAlrProbesUs{ 5 * 1000 * 1000 };
		// How far below what a burst aims for its result may land and still be worth
		// acting upon.
		static constexpr double ProbeUncertainty{ 0.05 };

		/* Instance methods. */

		ProbeController::ProbeController() : ProbeController(ProbeControllerOptions{})
		{
			MS_TRACE();
		}

		ProbeController::ProbeController(ProbeControllerOptions options) : options(options)
		{
			MS_TRACE();

			// The time between two shots of a burst is what its bitrate is held over to
			// decide how many bytes each shot carries, so none of them may be zero.
			MS_ASSERT(this->options.minProbeDeltaUs > 0, "minProbeDeltaUs cannot be zero");
			MS_ASSERT(this->options.initialMinProbeDeltaUs > 0, "initialMinProbeDeltaUs cannot be zero");
			MS_ASSERT(
			  this->options.networkStateMinProbeDeltaUs > 0, "networkStateMinProbeDeltaUs cannot be zero");

			// Until the application sets one, this is what bounds a burst.
			this->maxBitrate = DefaultMaxProbingBitrate;
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::SetBitrates(
		  int64_t minBitrate, int64_t startBitrate, int64_t maxBitrate, int64_t nowUs)
		{
			MS_TRACE();

			if (startBitrate > 0)
			{
				this->startBitrate     = startBitrate;
				this->estimatedBitrate = startBitrate;
			}
			else if (this->startBitrate == 0)
			{
				this->startBitrate = minBitrate;
			}

			// NOTE: Kept because the new maximum has to be in place before probing,
			// and the comparison below needs the old one.
			const int64_t oldMaxBitrate = this->maxBitrate;

			this->maxBitrate = maxBitrate != Types::BitrateInfinite ? maxBitrate : DefaultMaxProbingBitrate;

			switch (this->state)
			{
				case State::INIT:
				{
					if (this->networkAvailable)
					{
						return InitiateExponentialProbing(nowUs);
					}

					break;
				}

				case State::WAITING_FOR_PROBING_RESULT:
				{
					break;
				}

				case State::PROBING_COMPLETE:
				{
					// A maximum above both the previous one and the estimate is room that
					// hasn't been looked for yet.
					if (
					  this->estimatedBitrate != 0 && oldMaxBitrate < this->maxBitrate &&
					  this->estimatedBitrate < this->maxBitrate)
					{
						return InitiateProbing(nowUs, { this->maxBitrate }, /*probeFurther*/ false);
					}

					break;
				}
			}

			return {};
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::OnMaxTotalAllocatedBitrate(
		  int64_t maxTotalAllocatedBitrate, int64_t nowUs)
		{
			MS_TRACE();

			const bool inAlr = this->alrStartTimeUs.has_value();
			const bool allowAllocationProbe =
			  inAlr || this->options.probeOnMaxAllocatedBitrateChangeWithoutAlr;

			if (
			  this->options.probeOnMaxAllocatedBitrateChange && this->state == State::PROBING_COMPLETE &&
			  maxTotalAllocatedBitrate > this->maxTotalAllocatedBitrate &&
			  this->estimatedBitrate < this->maxBitrate &&
			  this->estimatedBitrate < maxTotalAllocatedBitrate && allowAllocationProbe)
			{
				this->maxTotalAllocatedBitrate = maxTotalAllocatedBitrate;

				if (!this->options.firstAllocationProbeScale.has_value())
				{
					return {};
				}

				int64_t firstProbeBitrate = BitrateUtils::ApplyBitrateFactor(
				  maxTotalAllocatedBitrate, this->options.firstAllocationProbeScale.value());
				const int64_t currentBweLimit = BitrateUtils::ApplyBitrateFactor(
				  this->estimatedBitrate, this->options.allocationProbeLimitByCurrentScale);

				bool limitedByCurrentBwe = currentBweLimit < firstProbeBitrate;

				if (limitedByCurrentBwe)
				{
					firstProbeBitrate = currentBweLimit;
				}

				std::vector<int64_t> probes{ firstProbeBitrate };

				if (!limitedByCurrentBwe && this->options.secondAllocationProbeScale.has_value())
				{
					int64_t secondProbeBitrate = BitrateUtils::ApplyBitrateFactor(
					  maxTotalAllocatedBitrate, this->options.secondAllocationProbeScale.value());

					limitedByCurrentBwe = currentBweLimit < secondProbeBitrate;

					if (limitedByCurrentBwe)
					{
						secondProbeBitrate = currentBweLimit;
					}

					if (secondProbeBitrate > firstProbeBitrate)
					{
						probes.push_back(secondProbeBitrate);
					}
				}

				return InitiateProbing(nowUs, probes, /*probeFurther*/ limitedByCurrentBwe);
			}

			// Having something to send is what ends the repeated initial probing, since
			// from then on there is traffic of its own to measure.
			if (maxTotalAllocatedBitrate != 0)
			{
				this->lastAllowedRepeatedInitialProbeAtUs = nowUs;
			}

			this->maxTotalAllocatedBitrate = maxTotalAllocatedBitrate;

			return {};
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::OnNetworkAvailability(
		  bool networkAvailable, int64_t nowUs)
		{
			MS_TRACE();

			this->networkAvailable = networkAvailable;

			if (!this->networkAvailable && this->state == State::WAITING_FOR_PROBING_RESULT)
			{
				this->state                    = State::PROBING_COMPLETE;
				this->minBitrateToProbeFurther = Types::BitrateInfinite;
			}

			if (this->networkAvailable && this->state == State::INIT && this->startBitrate != 0)
			{
				return InitiateExponentialProbing(nowUs);
			}

			return {};
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::SetEstimatedBitrate(
		  int64_t bitrate, BandwidthLimitedCause bandwidthLimitedCause, int64_t nowUs)
		{
			MS_TRACE();

			this->bandwidthLimitedCause = bandwidthLimitedCause;

			if (bitrate < BitrateUtils::ApplyBitrateFactor(this->estimatedBitrate, BitrateDropThreshold))
			{
				this->lastLargeDropAtUs          = nowUs;
				this->bitrateBeforeLastLargeDrop = this->estimatedBitrate;
			}

			this->estimatedBitrate = bitrate;

			if (this->state == State::WAITING_FOR_PROBING_RESULT)
			{
				// Reaching what was asked for leaves nothing to keep climbing towards.
				if (
				  this->options.abortFurtherProbeIfMaxLowerThanCurrent &&
				  (bitrate > this->maxBitrate ||
					 (this->maxTotalAllocatedBitrate != 0 &&
					  bitrate > BitrateUtils::ApplyBitrateFactor(this->maxTotalAllocatedBitrate, 2.0))))
				{
					this->minBitrateToProbeFurther = Types::BitrateInfinite;
				}

				const int64_t networkStateProbeFurtherLimit =
				  this->options.networkStateEstimateProbingIntervalUs != Types::TimeUsInfinite &&
				      this->linkCapacityUpperBound.has_value()
				    ? BitrateUtils::ApplyBitrateFactor(
				        this->linkCapacityUpperBound.value(), this->options.furtherProbeThreshold)
				    : Types::BitrateInfinite;

				if (bitrate > this->minBitrateToProbeFurther && bitrate <= networkStateProbeFurtherLimit)
				{
					return InitiateProbing(
					  nowUs,
					  { BitrateUtils::ApplyBitrateFactor(bitrate, this->options.furtherExponentialProbeScale) },
					  /*probeFurther*/ true);
				}
			}

			return {};
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::RequestProbe(int64_t nowUs)
		{
			MS_TRACE();

			// Asked for once the estimate has come back to normal after a large drop.
			// A single burst at what it was worth before tells a drop caused by a
			// competing flow or by the network changing from a passing one.
			const bool inAlr            = this->alrStartTimeUs.has_value();
			const bool alrEndedRecently = this->alrEndedTimeUs.has_value() &&
			                              nowUs - this->alrEndedTimeUs.value() < AlrEndedTimeoutUs;

			if (!inAlr && !alrEndedRecently && !this->options.rapidRecoveryExperiment)
			{
				return {};
			}

			if (this->state != State::PROBING_COMPLETE)
			{
				return {};
			}

			const int64_t suggestedProbe =
			  BitrateUtils::ApplyBitrateFactor(this->bitrateBeforeLastLargeDrop, ProbeFractionAfterDrop);
			const int64_t minExpectedProbeResult =
			  BitrateUtils::ApplyBitrateFactor(suggestedProbe, 1.0 - ProbeUncertainty);

			// NOTE: Never having dropped nor probed is not a reason to hold the burst
			// back, so a missing instant counts as long ago.
			const bool droppedRecently = this->lastLargeDropAtUs.has_value() &&
			                             nowUs - this->lastLargeDropAtUs.value() < BitrateDropTimeoutUs;
			const bool probedRecently =
			  this->lastBweDropProbingAtUs.has_value() &&
			  nowUs - this->lastBweDropProbingAtUs.value() <= MinTimeBetweenAlrProbesUs;

			if (minExpectedProbeResult > this->estimatedBitrate && droppedRecently && !probedRecently)
			{
				MS_DEBUG_DEV("large drop detected, probing [suggestedProbe:%" PRIi64 "]", suggestedProbe);

				this->lastBweDropProbingAtUs = nowUs;

				return InitiateProbing(nowUs, { suggestedProbe }, /*probeFurther*/ false);
			}

			return {};
		}

		void ProbeController::Reset(int64_t nowUs)
		{
			MS_TRACE();

			this->bandwidthLimitedCause    = BandwidthLimitedCause::DELAY_BASED_LIMITED;
			this->state                    = State::INIT;
			this->minBitrateToProbeFurther = Types::BitrateInfinite;
			this->lastProbingInitiatedAtUs.reset();
			this->estimatedBitrate = 0;
			this->linkCapacityUpperBound.reset();
			this->startBitrate = 0;
			this->maxBitrate   = DefaultMaxProbingBitrate;
			this->alrEndedTimeUs.reset();
			this->lastBweDropProbingAtUs     = nowUs;
			this->lastLargeDropAtUs          = nowUs;
			this->bitrateBeforeLastLargeDrop = 0;
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::Process(int64_t nowUs)
		{
			MS_TRACE();

			// NOTE: Never having probed counts as having waited long enough.
			if (!this->lastProbingInitiatedAtUs.has_value() || nowUs - this->lastProbingInitiatedAtUs.value() > MaxWaitingTimeForProbingResultUs)
			{
				if (this->state == State::WAITING_FOR_PROBING_RESULT)
				{
					MS_DEBUG_DEV("gave up waiting for the result of a probe");

					UpdateState(State::PROBING_COMPLETE);
				}
			}

			if (this->estimatedBitrate == 0 || this->state != State::PROBING_COMPLETE)
			{
				return {};
			}

			if (IsTimeForNextRepeatedInitialProbe(nowUs))
			{
				return InitiateProbing(
				  nowUs,
				  { BitrateUtils::ApplyBitrateFactor(
				    this->estimatedBitrate, this->options.firstExponentialProbeScale) },
				  /*probeFurther*/ true);
			}

			if (IsTimeForAlrProbe(nowUs) || IsTimeForNetworkStateProbe(nowUs))
			{
				return InitiateProbing(
				  nowUs,
				  { BitrateUtils::ApplyBitrateFactor(this->estimatedBitrate, this->options.alrProbeScale) },
				  /*probeFurther*/ true);
			}

			return {};
		}

		void ProbeController::UpdateState(State newState)
		{
			MS_TRACE();

			this->state = newState;

			// Nothing left to climb towards means no bitrate can be worth following up.
			if (newState == State::PROBING_COMPLETE)
			{
				this->minBitrateToProbeFurther = Types::BitrateInfinite;
			}
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::InitiateExponentialProbing(int64_t nowUs)
		{
			MS_TRACE();

			// NOTE: The three of them are guaranteed by every caller, which only gets
			// here with a network and a bitrate to start from and nothing probed yet.
			MS_ASSERT(this->networkAvailable, "network is not available");
			MS_ASSERT(this->state == State::INIT, "probing already started");
			MS_ASSERT(this->startBitrate > 0, "no bitrate to start from");

			std::vector<int64_t> probes{ BitrateUtils::ApplyBitrateFactor(
				this->startBitrate, this->options.firstExponentialProbeScale) };

			if (
			  this->options.secondExponentialProbeScale.has_value() &&
			  this->options.secondExponentialProbeScale.value() > 0)
			{
				probes.push_back(
				  BitrateUtils::ApplyBitrateFactor(
				    this->startBitrate, this->options.secondExponentialProbeScale.value()));
			}

			// Repeating the initial bursts only makes sense while there is nothing of
			// our own being sent to measure instead.
			if (this->repeatedInitialProbingEnabled && this->maxTotalAllocatedBitrate == 0)
			{
				this->lastAllowedRepeatedInitialProbeAtUs =
				  nowUs + this->options.repeatedInitialProbingTimePeriodUs;
			}

			return InitiateProbing(nowUs, probes, /*probeFurther*/ true);
		}

		std::vector<Types::ProbeClusterConfig> ProbeController::InitiateProbing(
		  int64_t nowUs, const std::vector<int64_t>& bitratesToProbe, bool probeFurther)
		{
			MS_TRACE();

			if (this->options.skipIfEstimateLargerThanFractionOfMax > 0)
			{
				const int64_t linkCapacityUpperBound =
				  this->linkCapacityUpperBound.value_or(Types::BitrateInfinite);
				const int64_t maxProbeBitrate =
				  this->maxTotalAllocatedBitrate == 0
				    ? this->maxBitrate
				    : std::min(
				        BitrateUtils::ApplyBitrateFactor(
				          this->maxTotalAllocatedBitrate, this->options.skipProbeMaxAllocatedScale),
				        this->maxBitrate);

				if (
				  std::min(linkCapacityUpperBound, this->estimatedBitrate) >
				  BitrateUtils::ApplyBitrateFactor(
				    maxProbeBitrate, this->options.skipIfEstimateLargerThanFractionOfMax))
				{
					UpdateState(State::PROBING_COMPLETE);

					return {};
				}
			}

			int64_t maxProbeBitrate = this->maxBitrate;

			// Aiming above what the application wants to send is allowed, since a
			// stream that bursts would otherwise have to climb once it's already
			// overshooting, and a burst tends to be received slightly below what it
			// aimed for.
			if (this->maxTotalAllocatedBitrate > 0)
			{
				maxProbeBitrate = std::min(
				  maxProbeBitrate, BitrateUtils::ApplyBitrateFactor(this->maxTotalAllocatedBitrate, 2.0));
			}

			switch (this->bandwidthLimitedCause)
			{
				case BandwidthLimitedCause::RTT_BASED_BACK_OFF_HIGH_RTT:
				case BandwidthLimitedCause::DELAY_BASED_LIMITED_DELAY_INCREASED:
				case BandwidthLimitedCause::LOSS_LIMITED_BWE:
				{
					// Something is already holding the estimate back, so a burst would
					// measure that and not the link.
					MS_DEBUG_DEV("not probing while the estimate is limited");

					return {};
				}

				case BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING:
				{
					maxProbeBitrate = std::min(
					  maxProbeBitrate,
					  BitrateUtils::ApplyBitrateFactor(
					    this->estimatedBitrate, this->options.lossLimitedProbeScale));

					break;
				}

				case BandwidthLimitedCause::DELAY_BASED_LIMITED:
				{
					break;
				}
			}

			if (
			  this->options.networkStateEstimateProbingIntervalUs != Types::TimeUsInfinite &&
			  this->linkCapacityUpperBound.has_value())
			{
				if (this->linkCapacityUpperBound.value() == 0)
				{
					MS_DEBUG_DEV("not probing, the link is known to carry nothing");

					return {};
				}

				maxProbeBitrate = std::min(
				  maxProbeBitrate,
				  std::max(
				    this->estimatedBitrate,
				    BitrateUtils::ApplyBitrateFactor(
				      this->linkCapacityUpperBound.value(), this->options.networkStateProbeScale)));
			}

			std::vector<Types::ProbeClusterConfig> pendingProbes;

			pendingProbes.reserve(bitratesToProbe.size());

			for (int64_t bitrate : bitratesToProbe)
			{
				// A burst at no bitrate is not a burst. It comes up when what bounds it
				// is an estimate that doesn't exist yet, and then there is nothing to
				// look for.
				if (bitrate == 0)
				{
					continue;
				}

				if (bitrate >= maxProbeBitrate)
				{
					bitrate      = maxProbeBitrate;
					probeFurther = false;
				}

				pendingProbes.push_back(CreateProbeClusterConfig(nowUs, bitrate));
			}

			this->lastProbingInitiatedAtUs = nowUs;

			// Nothing was asked for, so there is no result to wait for either.
			if (probeFurther && !pendingProbes.empty())
			{
				UpdateState(State::WAITING_FOR_PROBING_RESULT);

				// A burst is not expected to be received at the whole bitrate it was
				// sent at, so what has to be beaten is a fraction of it.
				this->minBitrateToProbeFurther = BitrateUtils::ApplyBitrateFactor(
				  pendingProbes.back().targetBitrate, this->options.furtherProbeThreshold);
			}
			else
			{
				UpdateState(State::PROBING_COMPLETE);
			}

			return pendingProbes;
		}

		bool ProbeController::IsTimeForAlrProbe(int64_t nowUs) const
		{
			MS_TRACE();

			if (!this->enablePeriodicAlrProbing || !this->alrStartTimeUs.has_value())
			{
				return false;
			}

			// NOTE: Never having probed makes the start of the state the only instant
			// to measure from.
			const int64_t sinceUs = std::max(
			  this->alrStartTimeUs.value(),
			  this->lastProbingInitiatedAtUs.value_or(this->alrStartTimeUs.value()));

			return nowUs >= sinceUs + this->options.alrProbingIntervalUs;
		}

		bool ProbeController::IsTimeForNetworkStateProbe(int64_t nowUs) const
		{
			MS_TRACE();

			if (!this->linkCapacityUpperBound.has_value())
			{
				return false;
			}

			const bool probeDueToLowEstimate =
			  this->bandwidthLimitedCause == BandwidthLimitedCause::DELAY_BASED_LIMITED &&
			  this->estimatedBitrate < BitrateUtils::ApplyBitrateFactor(
			                             this->linkCapacityUpperBound.value(),
			                             this->options.probeIfEstimateLowerThanNetworkStateEstimateRatio);

			// NOTE: Never having probed means that whichever interval applies has
			// already gone by.
			if (probeDueToLowEstimate && this->options.estimateLowerThanNetworkStateEstimateProbingIntervalUs != Types::TimeUsInfinite)
			{
				return !this->lastProbingInitiatedAtUs.has_value() ||
				       nowUs >= this->lastProbingInitiatedAtUs.value() +
				                  this->options.estimateLowerThanNetworkStateEstimateProbingIntervalUs;
			}

			const bool periodicProbe = this->estimatedBitrate < this->linkCapacityUpperBound.value();

			if (periodicProbe && this->options.networkStateEstimateProbingIntervalUs != Types::TimeUsInfinite)
			{
				return !this->lastProbingInitiatedAtUs.has_value() ||
				       nowUs >= this->lastProbingInitiatedAtUs.value() +
				                  this->options.networkStateEstimateProbingIntervalUs;
			}

			return false;
		}

		bool ProbeController::IsTimeForNextRepeatedInitialProbe(int64_t nowUs) const
		{
			MS_TRACE();

			if (
			  this->state == State::WAITING_FOR_PROBING_RESULT ||
			  !this->lastAllowedRepeatedInitialProbeAtUs.has_value() ||
			  this->lastAllowedRepeatedInitialProbeAtUs.value() <= nowUs)
			{
				return false;
			}

			// NOTE: Never having probed means that the wait is already over.
			return !this->lastProbingInitiatedAtUs.has_value() ||
			       nowUs >= this->lastProbingInitiatedAtUs.value() + MaxWaitingTimeForProbingResultUs;
		}

		Types::ProbeClusterConfig ProbeController::CreateProbeClusterConfig(int64_t nowUs, int64_t bitrate)
		{
			MS_TRACE();

			Types::ProbeClusterConfig config;

			config.atUs          = nowUs;
			config.targetBitrate = bitrate;

			// A burst aiming no higher than what the link is known to carry can be
			// shorter, since it isn't looking for room that may not be there.
			if (
			  this->linkCapacityUpperBound.has_value() &&
			  this->options.networkStateEstimateProbingIntervalUs != Types::TimeUsInfinite &&
			  this->linkCapacityUpperBound.value() >= bitrate)
			{
				config.targetDurationUs = this->options.networkStateProbeDurationUs;
				config.minProbeDeltaUs  = this->options.networkStateMinProbeDeltaUs;
			}
			else if (
			  this->lastAllowedRepeatedInitialProbeAtUs.has_value() &&
			  nowUs < this->lastAllowedRepeatedInitialProbeAtUs.value())
			{
				config.targetDurationUs = this->options.initialProbeDurationUs;
				config.minProbeDeltaUs  = this->options.initialMinProbeDeltaUs;
			}
			else
			{
				config.targetDurationUs = this->options.minProbeDurationUs;
				config.minProbeDeltaUs  = this->options.minProbeDeltaUs;
			}

			config.targetProbeCount = this->options.minProbePacketsSent;
			config.id               = this->nextProbeClusterId;

			this->nextProbeClusterId++;

			return config;
		}
	} // namespace BWE
} // namespace RTC
