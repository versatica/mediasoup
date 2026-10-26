#define MS_CLASS "RTC::BWE::LossBasedController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/LossBasedController.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BitrateUtils.hpp"
#include "RTC/Consts.hpp"
#include <cmath>
#include <limits>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How long the first hold lasts, each following one lasting longer.
		static constexpr int64_t InitHoldDurationUs{ 300 * 1000 };
		// How long a hold may last no matter how many have piled up.
		static constexpr int64_t MaxHoldDurationUs{ 60 * 1000 * 1000 };

		/**
		 * Loss to expect from a link described by the given pair of values while
		 * sending at the given rate: its inherent loss, plus the share of the traffic
		 * that doesn't fit while sending above its capacity.
		 */
		static double getLossProbability(double inherentLoss, int64_t lossLimitedBitrate, int64_t sendingRate)
		{
			MS_TRACE();

			// NOTE: Clamped rather than asserted because a candidate refined by
			// Newton's method can land outside before being brought back.
			inherentLoss = std::clamp(inherentLoss, 0.0, 1.0);

			double lossProbability{ inherentLoss };

			if (sendingRate > 0 && lossLimitedBitrate > 0 && sendingRate > lossLimitedBitrate)
			{
				lossProbability += (1 - inherentLoss) *
				                   static_cast<double>(sendingRate - lossLimitedBitrate) /
				                   static_cast<double>(sendingRate);
			}

			// Leave room on both ends, since the likelihood takes the logarithm of
			// this value and of its complement.
			return std::clamp(lossProbability, 1.0e-6, 1.0 - 1.0e-6);
		}

		/**
		 * Size in kilobytes, which is the unit the likelihood is computed in so that
		 * its terms stay in a sane range.
		 */
		static double toKiloBytes(int64_t bytes)
		{
			MS_TRACE();

			return static_cast<double>(bytes) / 1000.0;
		}

		/* Instance methods. */

		LossBasedController::LossBasedController() : LossBasedController(LossBasedControllerOptions{})
		{
			MS_TRACE();
		}

		LossBasedController::LossBasedController(LossBasedControllerOptions options)
		  : options(std::move(options))
		{
			MS_TRACE();

			// NOTE: Every option below is set from C++ alone, never from the network
			// nor from the API, so a value out of range can only be a programming
			// error. Ranges that are not checked here are the ones an option may
			// legitimately take to the extreme, such as a factor that is meant to be
			// disabled by setting it to zero.
			MS_ASSERT(
			  this->options.bitrateRampupUpperBoundFactor > 1.0,
			  "bitrate rampup upper bound factor must be greater than 1 [value:%f]",
			  this->options.bitrateRampupUpperBoundFactor);
			MS_ASSERT(
			  this->options.bitrateRampupUpperBoundFactorInHold > 1.0,
			  "bitrate rampup upper bound factor in hold must be greater than 1 [value:%f]",
			  this->options.bitrateRampupUpperBoundFactorInHold);
			MS_ASSERT(
			  this->options.bitrateRampupHoldThreshold >= 0.0,
			  "bitrate rampup hold threshold must not be negative [value:%f]",
			  this->options.bitrateRampupHoldThreshold);
			MS_ASSERT(
			  this->options.rampupAccelerationMaxFactor >= 0.0,
			  "rampup acceleration max factor must not be negative [value:%f]",
			  this->options.rampupAccelerationMaxFactor);
			MS_ASSERT(
			  this->options.rampupAccelerationMaxoutTimeUs > 0,
			  "rampup acceleration maxout time must be positive [value:%" PRIi64 "]",
			  this->options.rampupAccelerationMaxoutTimeUs);

			for (const auto candidateFactor : this->options.candidateFactors)
			{
				MS_ASSERT(
				  candidateFactor > 0.0, "candidate factors must be positive [value:%f]", candidateFactor);
			}

			// At least one candidate other than the current estimate must be
			// reachable, or there is nothing for an update to choose from.
			MS_ASSERT(
			  this->options.appendAcknowledgedRateCandidate ||
			    this->options.appendDelayBasedEstimateCandidate ||
			    std::ranges::any_of(
			      this->options.candidateFactors,
			      [](double candidateFactor) -> bool
			      {
				      return candidateFactor != 1.0;
			      }),
			  "no candidates can be generated, so give a candidate factor other than 1, or let the "
			  "acknowledged bitrate or the delay based estimate be candidates");

			MS_ASSERT(
			  this->options.higherBitrateBiasFactor >= 0.0,
			  "higher bitrate bias factor must not be negative [value:%f]",
			  this->options.higherBitrateBiasFactor);
			MS_ASSERT(
			  this->options.inherentLossLowerBound >= 0.0 && this->options.inherentLossLowerBound < 1.0,
			  "inherent loss lower bound must be in [0, 1) [value:%f]",
			  this->options.inherentLossLowerBound);
			MS_ASSERT(
			  this->options.lossThresholdOfHighBitratePreference >= 0.0 &&
			    this->options.lossThresholdOfHighBitratePreference < 1.0,
			  "loss threshold of high bitrate preference must be in [0, 1) [value:%f]",
			  this->options.lossThresholdOfHighBitratePreference);
			MS_ASSERT(
			  this->options.bitratePreferenceSmoothingFactor > 0.0 &&
			    this->options.bitratePreferenceSmoothingFactor <= 1.0,
			  "bitrate preference smoothing factor must be in (0, 1] [value:%f]",
			  this->options.bitratePreferenceSmoothingFactor);
			MS_ASSERT(
			  this->options.inherentLossUpperBoundBitrateBalance > 0,
			  "inherent loss upper bound bitrate balance must be positive [value:%" PRIi64 "]",
			  this->options.inherentLossUpperBoundBitrateBalance);
			MS_ASSERT(
			  this->options.inherentLossUpperBoundOffset >= this->options.inherentLossLowerBound &&
			    this->options.inherentLossUpperBoundOffset < 1.0,
			  "inherent loss upper bound offset must be in [inherentLossLowerBound, 1) [value:%f, "
			  "inherentLossLowerBound:%f]",
			  this->options.inherentLossUpperBoundOffset,
			  this->options.inherentLossLowerBound);
			MS_ASSERT(
			  this->options.initialInherentLossEstimate >= 0.0 &&
			    this->options.initialInherentLossEstimate < 1.0,
			  "initial inherent loss estimate must be in [0, 1) [value:%f]",
			  this->options.initialInherentLossEstimate);
			MS_ASSERT(
			  this->options.newtonIterations > 0,
			  "number of Newton iterations must be positive [value:%" PRIi64 "]",
			  this->options.newtonIterations);
			MS_ASSERT(
			  this->options.newtonStepSize > 0.0,
			  "Newton step size must be positive [value:%f]",
			  this->options.newtonStepSize);
			MS_ASSERT(
			  this->options.observationDurationLowerBoundUs > 0,
			  "observation duration lower bound must be positive [value:%" PRIi64 "]",
			  this->options.observationDurationLowerBoundUs);
			// NOTE: The window is also what every observation is indexed modulo, so an
			// empty one would divide by zero.
			MS_ASSERT(
			  this->options.observationWindowSize >= 2,
			  "observation window size must be at least 2 [value:%" PRIi64 "]",
			  this->options.observationWindowSize);
			MS_ASSERT(
			  this->options.sendingRateSmoothingFactor >= 0.0 &&
			    this->options.sendingRateSmoothingFactor < 1.0,
			  "sending rate smoothing factor must be in [0, 1) [value:%f]",
			  this->options.sendingRateSmoothingFactor);
			MS_ASSERT(
			  this->options.immediateUpperBoundTemporalWeightFactor > 0.0 &&
			    this->options.immediateUpperBoundTemporalWeightFactor <= 1.0,
			  "immediate upper bound temporal weight factor must be in (0, 1] [value:%f]",
			  this->options.immediateUpperBoundTemporalWeightFactor);
			MS_ASSERT(
			  this->options.immediateUpperBoundBitrateBalance > 0,
			  "immediate upper bound bitrate balance must be positive [value:%" PRIi64 "]",
			  this->options.immediateUpperBoundBitrateBalance);
			MS_ASSERT(
			  this->options.immediateUpperBoundLossOffset >= 0.0 &&
			    this->options.immediateUpperBoundLossOffset < 1.0,
			  "immediate upper bound loss offset must be in [0, 1) [value:%f]",
			  this->options.immediateUpperBoundLossOffset);
			MS_ASSERT(
			  this->options.temporalWeightFactor > 0.0 && this->options.temporalWeightFactor <= 1.0,
			  "temporal weight factor must be in (0, 1] [value:%f]",
			  this->options.temporalWeightFactor);
			MS_ASSERT(
			  this->options.bitrateBackoffLowerBoundFactor <= 1.0,
			  "bitrate backoff lower bound factor must not be greater than 1 [value:%f]",
			  this->options.bitrateBackoffLowerBoundFactor);
			MS_ASSERT(
			  this->options.maxIncreaseFactor > 0.0,
			  "max increase factor must be positive [value:%f]",
			  this->options.maxIncreaseFactor);
			MS_ASSERT(
			  this->options.delayedIncreaseWindowUs > 0,
			  "delayed increase window must be positive [value:%" PRIi64 "]",
			  this->options.delayedIncreaseWindowUs);
			MS_ASSERT(
			  this->options.minNumObservations > 0,
			  "min number of observations must be positive [value:%" PRIi64 "]",
			  this->options.minNumObservations);
			MS_ASSERT(
			  this->options.lowerBoundByAckedRateFactor >= 0.0,
			  "lower bound by acknowledged rate factor must not be negative [value:%f]",
			  this->options.lowerBoundByAckedRateFactor);

			this->currentBestEstimate.inherentLoss = this->options.initialInherentLossEstimate;

			this->observations.resize(this->options.observationWindowSize);
			this->temporalWeights.resize(this->options.observationWindowSize);
			this->immediateUpperBoundTemporalWeights.resize(this->options.observationWindowSize);

			CalculateTemporalWeights();

			this->lastHoldInfo.durationUs = InitHoldDurationUs;
		}

		bool LossBasedController::IsReady() const
		{
			MS_TRACE();

			return this->currentBestEstimate.lossLimitedBitrate > 0 &&
			       this->numObservations >= this->options.minNumObservations;
		}

		bool LossBasedController::IsReadyToUseInStartPhase() const
		{
			MS_TRACE();

			return IsReady() && this->options.useInStartPhase;
		}

		void LossBasedController::Reset()
		{
			MS_TRACE();

			// Everything the constructor leaves behind, since what was observed
			// describes a link that is not the one being used anymore. The temporal
			// weights are left alone because they only depend on the options.
			this->observations.assign(this->options.observationWindowSize, Observation{});
			this->partialObservation = {};

			this->currentBestEstimate              = {};
			this->currentBestEstimate.inherentLoss = this->options.initialInherentLossEstimate;

			this->lastHoldInfo            = {};
			this->lastHoldInfo.durationUs = InitHoldDurationUs;

			this->result          = {};
			this->numObservations = 0;
			this->lastSendTimeOfLatestObservationUs.reset();
			this->averageReportedLossRatio = 0.0;
			this->acknowledgedBitrate      = Types::BitrateInfinite;
			this->delayBasedEstimate       = Types::BitrateInfinite;
			this->minBitrate               = 0;
			this->maxBitrate               = Types::BitrateInfinite;
			this->immediateUpperBoundBitrate.reset();
			this->immediateLowerBoundBitrate.reset();
			this->bitrateLimitInCurrentWindow = Types::BitrateInfinite;
			this->recoveringAfterLossAtUs.reset();
			this->lastBitrateReducedAtUs.reset();
		}

		LossBasedController::Result LossBasedController::GetResult() const
		{
			MS_TRACE();

			if (!IsReady())
			{
				return { .bitrate = this->delayBasedEstimate, .state = State::DELAY_BASED_ESTIMATE };
			}

			return this->result;
		}

		void LossBasedController::SetAcknowledgedBitrate(int64_t acknowledgedBitrate)
		{
			MS_TRACE();

			this->acknowledgedBitrate = acknowledgedBitrate;

			CalculateImmediateLowerBoundBitrate();
		}

		void LossBasedController::SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate)
		{
			MS_TRACE();

			this->minBitrate = minBitrate;
			this->maxBitrate = maxBitrate;

			CalculateImmediateLowerBoundBitrate();
		}

		void LossBasedController::SetBitrateEstimate(int64_t bitrate)
		{
			MS_TRACE();

			this->currentBestEstimate.lossLimitedBitrate = bitrate;
			this->result = { .bitrate = bitrate, .state = State::DELAY_BASED_ESTIMATE };
		}

		void LossBasedController::UpdateBitrateEstimate(
		  const std::vector<Types::PacketResult>& packetResults, int64_t delayBasedEstimate, bool inAlr)
		{
			MS_TRACE();

			this->delayBasedEstimate = delayBasedEstimate;

			if (!PushBackObservation(packetResults) || !this->lastSendTimeOfLatestObservationUs.has_value())
			{
				return;
			}

			const int64_t lastSendTimeUs = this->lastSendTimeOfLatestObservationUs.value();

			// Nothing has been estimated yet, so the delay based path is the only
			// thing that can say where to start from.
			if (this->currentBestEstimate.lossLimitedBitrate == 0)
			{
				if (delayBasedEstimate == Types::BitrateInfinite)
				{
					MS_WARN_TAG(bwe, "no delay based estimate to start the loss based one from");

					return;
				}

				this->currentBestEstimate.lossLimitedBitrate = delayBasedEstimate;
				this->result = { .bitrate = delayBasedEstimate, .state = State::DELAY_BASED_ESTIMATE };
			}

			// Take the candidate whose pair of values makes the observed losses most
			// likely, preferring the higher bitrate when two are equally likely.
			ChannelParameters bestCandidate = this->currentBestEstimate;
			double maxObjective{ std::numeric_limits<double>::lowest() };

			for (ChannelParameters candidate : GetCandidates(inAlr))
			{
				ApplyNewtonsMethod(candidate);

				const double candidateObjective = GetObjective(candidate);

				if (
				  candidateObjective > maxObjective ||
				  (candidateObjective == maxObjective &&
					 candidate.lossLimitedBitrate > bestCandidate.lossLimitedBitrate))
				{
					maxObjective  = candidateObjective;
					bestCandidate = candidate;
				}
			}

			if (bestCandidate.lossLimitedBitrate < this->currentBestEstimate.lossLimitedBitrate)
			{
				this->lastBitrateReducedAtUs = lastSendTimeUs;
			}

			// Don't increase the estimate while the loss observed is worse than the
			// one the chosen candidate says the link has by itself, since then the
			// excess is ours to fix.
			if (
			  this->averageReportedLossRatio > bestCandidate.inherentLoss &&
			  this->options.notIncreaseIfInherentLossLessThanAverageLoss &&
			  this->currentBestEstimate.lossLimitedBitrate < bestCandidate.lossLimitedBitrate)
			{
				bestCandidate.lossLimitedBitrate = this->currentBestEstimate.lossLimitedBitrate;
			}

			if (IsInLossLimitedState())
			{
				// Right after a decrease the estimate may not grow past what that
				// decrease allowed, so that it doesn't jump straight back to the bitrate
				// that caused the loss.
				if (
				  this->recoveringAfterLossAtUs.has_value() &&
				  this->recoveringAfterLossAtUs.value() + this->options.delayedIncreaseWindowUs >
				    lastSendTimeUs &&
				  bestCandidate.lossLimitedBitrate > this->bitrateLimitInCurrentWindow)
				{
					bestCandidate.lossLimitedBitrate = this->bitrateLimitInCurrentWindow;
				}

				const bool increasingWhenLossLimited = IsEstimateIncreasingWhenLossLimited(
				  this->currentBestEstimate.lossLimitedBitrate, bestCandidate.lossLimitedBitrate);

				// While loss limited, growing beyond what the link is known to be
				// delivering is a guess, so it's bounded by the acknowledged bitrate.
				if (increasingWhenLossLimited && this->acknowledgedBitrate != Types::BitrateInfinite)
				{
					double rampupFactor{ this->options.bitrateRampupUpperBoundFactor };

					if (
					  this->lastHoldInfo.bitrate != Types::BitrateInfinite &&
					  this->acknowledgedBitrate <
					    BitrateUtils::ApplyBitrateFactor(
					      this->lastHoldInfo.bitrate, this->options.bitrateRampupHoldThreshold))
					{
						rampupFactor = this->options.bitrateRampupUpperBoundFactorInHold;
					}

					bestCandidate.lossLimitedBitrate = std::max(
					  this->currentBestEstimate.lossLimitedBitrate,
					  std::min(
					    bestCandidate.lossLimitedBitrate,
					    BitrateUtils::ApplyBitrateFactor(this->acknowledgedBitrate, rampupFactor)));

					// Growing by a single bit is what lets the state stop being
					// decreasing. Without it, a bound that leaves the estimate untouched
					// keeps it decreasing for good.
					if (
					  this->result.state == State::DECREASING &&
					  bestCandidate.lossLimitedBitrate == this->currentBestEstimate.lossLimitedBitrate)
					{
						bestCandidate.lossLimitedBitrate =
						  BitrateUtils::AddBitrates(this->currentBestEstimate.lossLimitedBitrate, 1);
					}
				}
			}

			const int64_t boundedBitrate = std::max(
			  GetImmediateLowerBoundBitrate(),
			  std::min(
			    { bestCandidate.lossLimitedBitrate,
					  GetImmediateUpperBoundBitrate(),
					  this->delayBasedEstimate }));

			// A bitrate that had to be brought down to its bounds is not an estimate
			// of the link anymore, so the pair of values is not kept as such.
			if (this->options.boundBestCandidate && boundedBitrate < bestCandidate.lossLimitedBitrate)
			{
				this->currentBestEstimate.lossLimitedBitrate = boundedBitrate;
				this->currentBestEstimate.inherentLoss       = 0;
			}
			else
			{
				this->currentBestEstimate = bestCandidate;

				// A link is never worth describing as delivering less than what it is
				// known to be delivering, unless that bound has been turned off.
				if (this->options.lowerBoundByAckedRateFactor > 0.0)
				{
					this->currentBestEstimate.lossLimitedBitrate =
					  std::max(this->currentBestEstimate.lossLimitedBitrate, GetImmediateLowerBoundBitrate());
				}
			}

			// While holding, the estimate may not grow above the bitrate being held,
			// which is the whole point of holding.
			if (
			  this->result.state == State::DECREASING && this->lastHoldInfo.atUs > lastSendTimeUs &&
			  boundedBitrate < this->delayBasedEstimate)
			{
				// A link is never worth holding below what it is known to be
				// delivering, unless that bound has been turned off.
				if (this->options.lowerBoundByAckedRateFactor > 0.0)
				{
					this->lastHoldInfo.bitrate =
					  std::max(GetImmediateLowerBoundBitrate(), this->lastHoldInfo.bitrate);
				}

				this->result.bitrate = std::min(this->lastHoldInfo.bitrate, boundedBitrate);

				return;
			}

			if (
			  IsEstimateIncreasingWhenLossLimited(this->result.bitrate, boundedBitrate) &&
			  boundedBitrate < this->delayBasedEstimate && boundedBitrate < this->maxBitrate)
			{
				this->result.state = State::INCREASING;
			}
			else if (boundedBitrate < this->delayBasedEstimate && boundedBitrate < this->maxBitrate)
			{
				// A factor of zero means that no hold is ever entered, since each hold
				// would last no time at all.
				if (this->result.state != State::DECREASING && this->options.holdDurationFactor > 0.0)
				{
					MS_DEBUG_DEV(
					  "switching to hold [bitrate:%" PRIi64 ", durationMs:%" PRIi64 ", averageLoss:%f]",
					  boundedBitrate,
					  this->lastHoldInfo.durationUs / 1000,
					  this->averageReportedLossRatio);

					this->lastHoldInfo = { .atUs       = lastSendTimeUs + this->lastHoldInfo.durationUs,
					                       .durationUs = std::min<int64_t>(
						                       MaxHoldDurationUs,
						                       std::llround(
						                         static_cast<double>(this->lastHoldInfo.durationUs) *
						                         this->options.holdDurationFactor)),
					                       .bitrate = boundedBitrate };
				}

				this->result.state = State::DECREASING;
			}
			else
			{
				// The delay based path is what limits us, so any hold that was in place
				// would only keep the estimate low for no reason.
				this->lastHoldInfo = { .atUs       = 0,
				                       .durationUs = InitHoldDurationUs,
				                       .bitrate    = Types::BitrateInfinite };

				this->result.state = State::DELAY_BASED_ESTIMATE;
			}

			this->result.bitrate = boundedBitrate;

			// Open a new window in which the estimate is not allowed to grow freely,
			// so that a link that keeps losing packets is approached gradually.
			if (
			  IsInLossLimitedState() &&
			  (!this->recoveringAfterLossAtUs.has_value() ||
				 this->recoveringAfterLossAtUs.value() + this->options.delayedIncreaseWindowUs <
				   lastSendTimeUs))
			{
				this->bitrateLimitInCurrentWindow = std::max<int64_t>(
				  Consts::BweMinBitrate,
				  BitrateUtils::ApplyBitrateFactor(
				    this->currentBestEstimate.lossLimitedBitrate, this->options.maxIncreaseFactor));

				this->recoveringAfterLossAtUs = lastSendTimeUs;
			}
		}

		bool LossBasedController::PushBackObservation(const std::vector<Types::PacketResult>& packetResults)
		{
			MS_TRACE();

			if (packetResults.empty())
			{
				return false;
			}

			int64_t firstSendTimeUs{ Types::TimeUsInfinite };
			int64_t lastSendTimeUs{ 0 };
			bool anySendTime{ false };

			for (const auto& packetResult : packetResults)
			{
				const auto sendTimeUs = packetResult.sentPacket.sendTimeUs;

				// A packet with no send time belongs to no span of send times, and its
				// bytes cannot be counted either: the duration of the observation comes
				// from the instants of the other packets, so adding its size would
				// inflate the sending rate attributed to them.
				if (sendTimeUs == Types::TimeUsInfinite)
				{
					continue;
				}

				if (packetResult.IsReceived())
				{
					this->partialObservation.lostPackets.erase(packetResult.sentPacket.sequenceNumber);
				}
				else
				{
					this->partialObservation.lostPackets.emplace(
					  packetResult.sentPacket.sequenceNumber,
					  static_cast<int64_t>(packetResult.sentPacket.size));
				}

				this->partialObservation.numPackets += 1;
				this->partialObservation.sizeBytes += static_cast<int64_t>(packetResult.sentPacket.size);

				firstSendTimeUs = std::min(firstSendTimeUs, sendTimeUs);
				lastSendTimeUs  = std::max(lastSendTimeUs, sendTimeUs);
				anySendTime     = true;
			}

			if (!anySendTime)
			{
				return false;
			}

			// This is the first feedback ever received.
			if (!this->lastSendTimeOfLatestObservationUs.has_value())
			{
				this->lastSendTimeOfLatestObservationUs = firstSendTimeUs;
			}

			const int64_t observationDurationUs =
			  lastSendTimeUs - this->lastSendTimeOfLatestObservationUs.value();

			// Too short a span to tell anything.
			if (observationDurationUs <= 0 || observationDurationUs < this->options.observationDurationLowerBoundUs)
			{
				return false;
			}

			this->lastSendTimeOfLatestObservationUs = lastSendTimeUs;

			int64_t lostSizeBytes{ 0 };

			for (const auto& kv : this->partialObservation.lostPackets)
			{
				lostSizeBytes += kv.second;
			}

			Observation observation;

			observation.numPackets = this->partialObservation.numPackets;
			observation.numLostPackets = static_cast<int64_t>(this->partialObservation.lostPackets.size());
			observation.numReceivedPackets = observation.numPackets - observation.numLostPackets;
			observation.sizeBytes          = this->partialObservation.sizeBytes;
			observation.lostSizeBytes      = lostSizeBytes;
			observation.sendingRate =
			  GetSendingRate((this->partialObservation.sizeBytes * 8 * 1000000) / observationDurationUs);
			observation.id = this->numObservations++;

			this->observations[observation.id % this->options.observationWindowSize] = observation;

			this->partialObservation = PartialObservation();

			UpdateAverageReportedLossRatio();
			CalculateImmediateUpperBoundBitrate();

			return true;
		}

		std::vector<LossBasedController::ChannelParameters> LossBasedController::GetCandidates(bool inAlr) const
		{
			MS_TRACE();

			std::vector<int64_t> bitrates;

			// The factors plus the acknowledged bitrate, the delay based estimate and
			// the bound that the observed loss puts on the estimate.
			bitrates.reserve(this->options.candidateFactors.size() + 3);

			for (const double candidateFactor : this->options.candidateFactors)
			{
				bitrates.push_back(
				  BitrateUtils::ApplyBitrateFactor(
				    this->currentBestEstimate.lossLimitedBitrate, candidateFactor));
			}

			// While not sending enough to fill the link, what it delivers says nothing
			// about what it could deliver, so it's no candidate.
			if (
			  this->acknowledgedBitrate != Types::BitrateInfinite &&
			  this->options.appendAcknowledgedRateCandidate &&
			  !(this->options.notUseAckedRateInAlr && inAlr))
			{
				bitrates.push_back(
				  BitrateUtils::ApplyBitrateFactor(
				    this->acknowledgedBitrate, this->options.bitrateBackoffLowerBoundFactor));
			}

			if (
			  this->delayBasedEstimate != Types::BitrateInfinite &&
			  this->options.appendDelayBasedEstimateCandidate &&
			  this->delayBasedEstimate > this->currentBestEstimate.lossLimitedBitrate)
			{
				bitrates.push_back(this->delayBasedEstimate);
			}

			// Coming all the way down to that bound in one step rather than gradually
			// only makes sense while not filling the link, since then the loss is not
			// ours to fix by sending less.
			if (
			  inAlr && this->options.appendUpperBoundCandidateInAlr &&
			  this->currentBestEstimate.lossLimitedBitrate > GetImmediateUpperBoundBitrate())
			{
				bitrates.push_back(GetImmediateUpperBoundBitrate());
			}

			const int64_t candidateBitrateUpperBound = GetCandidateBitrateUpperBound();

			std::vector<ChannelParameters> candidates;

			candidates.reserve(bitrates.size());

			for (const int64_t bitrate : bitrates)
			{
				ChannelParameters candidate = this->currentBestEstimate;

				candidate.lossLimitedBitrate = std::min(
				  bitrate,
				  std::max(this->currentBestEstimate.lossLimitedBitrate, candidateBitrateUpperBound));
				candidate.inherentLoss = GetFeasibleInherentLoss(candidate);

				candidates.push_back(candidate);
			}

			return candidates;
		}

		int64_t LossBasedController::GetCandidateBitrateUpperBound() const
		{
			MS_TRACE();

			int64_t candidateBitrateUpperBound{ this->maxBitrate };

			if (IsInLossLimitedState() && this->bitrateLimitInCurrentWindow != Types::BitrateInfinite)
			{
				candidateBitrateUpperBound = this->bitrateLimitInCurrentWindow;
			}

			if (this->acknowledgedBitrate == Types::BitrateInfinite)
			{
				return candidateBitrateUpperBound;
			}

			// The longer it's been since the estimate was last reduced, the more it is
			// allowed to grow beyond its usual bound.
			if (this->options.rampupAccelerationMaxFactor > 0.0)
			{
				// Never having been reduced counts as the longest time possible, which
				// is what gives the fullest acceleration.
				const int64_t sinceBitrateReducedUs =
				  this->lastBitrateReducedAtUs.has_value() &&
				      this->lastSendTimeOfLatestObservationUs.has_value()
				    ? std::min(
				        this->options.rampupAccelerationMaxoutTimeUs,
				        std::max<int64_t>(
				          this->lastSendTimeOfLatestObservationUs.value() -
				            this->lastBitrateReducedAtUs.value(),
				          0))
				    : this->options.rampupAccelerationMaxoutTimeUs;
				const double rampupAcceleration =
				  this->options.rampupAccelerationMaxFactor * static_cast<double>(sinceBitrateReducedUs) /
				  static_cast<double>(this->options.rampupAccelerationMaxoutTimeUs);

				candidateBitrateUpperBound = BitrateUtils::AddBitrates(
				  candidateBitrateUpperBound,
				  BitrateUtils::ApplyBitrateFactor(this->acknowledgedBitrate, rampupAcceleration));
			}

			return candidateBitrateUpperBound;
		}

		double LossBasedController::GetObjective(const ChannelParameters& channelParameters) const
		{
			MS_TRACE();

			double objective{ 0.0 };

			const double highBitrateBias = GetHighBitrateBias(channelParameters.lossLimitedBitrate);

			for (const auto& observation : this->observations)
			{
				if (!observation.IsInitialized())
				{
					continue;
				}

				const double lossProbability = getLossProbability(
				  channelParameters.inherentLoss,
				  channelParameters.lossLimitedBitrate,
				  observation.sendingRate);

				const double temporalWeight =
				  this->temporalWeights[(this->numObservations - 1) - observation.id];

				if (this->options.useByteLossRate)
				{
					objective +=
					  temporalWeight * ((toKiloBytes(observation.lostSizeBytes) * std::log(lossProbability)) +
						                  (toKiloBytes(observation.sizeBytes - observation.lostSizeBytes) *
						                   std::log(1.0 - lossProbability)));
					objective += temporalWeight * highBitrateBias * toKiloBytes(observation.sizeBytes);
				}
				else
				{
					objective +=
					  temporalWeight *
					  ((static_cast<double>(observation.numLostPackets) * std::log(lossProbability)) +
						 (static_cast<double>(observation.numReceivedPackets) * std::log(1.0 - lossProbability)));
					objective += temporalWeight * highBitrateBias * static_cast<double>(observation.numPackets);
				}
			}

			return objective;
		}

		LossBasedController::Derivatives LossBasedController::GetDerivatives(
		  const ChannelParameters& channelParameters) const
		{
			MS_TRACE();

			Derivatives derivatives;

			for (const auto& observation : this->observations)
			{
				if (!observation.IsInitialized())
				{
					continue;
				}

				const double lossProbability = getLossProbability(
				  channelParameters.inherentLoss,
				  channelParameters.lossLimitedBitrate,
				  observation.sendingRate);

				const double temporalWeight =
				  this->temporalWeights[(this->numObservations - 1) - observation.id];

				if (this->options.useByteLossRate)
				{
					derivatives.first +=
					  temporalWeight * ((toKiloBytes(observation.lostSizeBytes) / lossProbability) -
						                  (toKiloBytes(observation.sizeBytes - observation.lostSizeBytes) /
						                   (1.0 - lossProbability)));
					derivatives.second -=
					  temporalWeight * ((toKiloBytes(observation.lostSizeBytes) / std::pow(lossProbability, 2)) +
						                  (toKiloBytes(observation.sizeBytes - observation.lostSizeBytes) /
						                   std::pow(1.0 - lossProbability, 2)));
				}
				else
				{
					derivatives.first +=
					  temporalWeight *
					  ((static_cast<double>(observation.numLostPackets) / lossProbability) -
						 (static_cast<double>(observation.numReceivedPackets) / (1.0 - lossProbability)));
					derivatives.second -=
					  temporalWeight *
					  ((static_cast<double>(observation.numLostPackets) / std::pow(lossProbability, 2)) +
						 (static_cast<double>(observation.numReceivedPackets) /
						  std::pow(1.0 - lossProbability, 2)));
				}
			}

			// NOTE: The second derivative is negative by construction, so a value of
			// zero can only come from rounding and would make the step below divide by
			// zero.
			if (derivatives.second >= 0.0)
			{
				derivatives.second = -1.0e-6;
			}

			return derivatives;
		}

		void LossBasedController::ApplyNewtonsMethod(ChannelParameters& channelParameters) const
		{
			MS_TRACE();

			if (this->numObservations <= 0)
			{
				return;
			}

			for (int64_t i{ 0 }; i < this->options.newtonIterations; ++i)
			{
				const Derivatives derivatives = GetDerivatives(channelParameters);

				channelParameters.inherentLoss -=
				  this->options.newtonStepSize * derivatives.first / derivatives.second;
				channelParameters.inherentLoss = GetFeasibleInherentLoss(channelParameters);
			}
		}

		double LossBasedController::GetFeasibleInherentLoss(const ChannelParameters& channelParameters) const
		{
			MS_TRACE();

			return std::min(
			  std::max(channelParameters.inherentLoss, this->options.inherentLossLowerBound),
			  GetInherentLossUpperBound(channelParameters.lossLimitedBitrate));
		}

		double LossBasedController::GetInherentLossUpperBound(int64_t bitrate) const
		{
			MS_TRACE();

			if (bitrate == 0)
			{
				return 1.0;
			}

			// The lower the bitrate, the more loss a link is allowed to be said to
			// have by itself, since at a low enough bitrate the traffic cannot be the
			// cause.
			const double inherentLossUpperBound =
			  this->options.inherentLossUpperBoundOffset +
			  (static_cast<double>(this->options.inherentLossUpperBoundBitrateBalance) /
				 static_cast<double>(bitrate));

			return std::min(inherentLossUpperBound, 1.0);
		}

		double LossBasedController::AdjustBiasFactor(double lossRate, double biasFactor) const
		{
			MS_TRACE();

			return biasFactor * (this->options.lossThresholdOfHighBitratePreference - lossRate) /
			       (this->options.bitratePreferenceSmoothingFactor +
			        std::abs(this->options.lossThresholdOfHighBitratePreference - lossRate));
		}

		double LossBasedController::GetHighBitrateBias(int64_t bitrate) const
		{
			MS_TRACE();

			if (bitrate <= 0)
			{
				return 0.0;
			}

			// NOTE: The bias is measured over a bitrate expressed with a resolution of
			// 1 kbps, so that it doesn't move with a fraction of it.
			const double bitrateKbps = std::round(static_cast<double>(bitrate) / 1000.0);

			return (AdjustBiasFactor(this->averageReportedLossRatio, this->options.higherBitrateBiasFactor) *
			        bitrateKbps) +
			       (AdjustBiasFactor(
			          this->averageReportedLossRatio, this->options.higherLogBitrateBiasFactor) *
			        std::log(1.0 + bitrateKbps));
		}

		int64_t LossBasedController::GetSendingRate(int64_t instantSendingRate) const
		{
			MS_TRACE();

			if (this->numObservations <= 0)
			{
				return instantSendingRate;
			}

			const int64_t latestObservationIdx =
			  (this->numObservations - 1) % this->options.observationWindowSize;
			const int64_t previousSendingRate = this->observations[latestObservationIdx].sendingRate;

			// Each share is rounded on its own before they are added, so that the
			// smoothed rate doesn't depend on how the two of them happen to split.
			return BitrateUtils::AddBitrates(
			  BitrateUtils::ApplyBitrateFactor(
			    previousSendingRate, this->options.sendingRateSmoothingFactor),
			  BitrateUtils::ApplyBitrateFactor(
			    instantSendingRate, 1.0 - this->options.sendingRateSmoothingFactor));
		}

		void LossBasedController::UpdateAverageReportedLossRatio()
		{
			MS_TRACE();

			this->averageReportedLossRatio = this->options.useByteLossRate
			                                   ? CalculateAverageReportedByteLossRatio()
			                                   : CalculateAverageReportedPacketLossRatio();
		}

		double LossBasedController::CalculateAverageReportedPacketLossRatio() const
		{
			MS_TRACE();

			if (this->numObservations <= 0)
			{
				return 0.0;
			}

			double numPackets{ 0.0 };
			double numLostPackets{ 0.0 };

			for (const auto& observation : this->observations)
			{
				if (!observation.IsInitialized())
				{
					continue;
				}

				const double temporalWeight =
				  this->immediateUpperBoundTemporalWeights[(this->numObservations - 1) - observation.id];

				numPackets += temporalWeight * static_cast<double>(observation.numPackets);
				numLostPackets += temporalWeight * static_cast<double>(observation.numLostPackets);
			}

			// NOTE: Nothing was sent at all, so there is no loss to tell. Without this
			// the division would give a NaN, which makes every comparison it takes
			// part in false and silently freezes the estimate for good.
			if (numPackets == 0.0)
			{
				return 0.0;
			}

			return numLostPackets / numPackets;
		}

		double LossBasedController::CalculateAverageReportedByteLossRatio() const
		{
			MS_TRACE();

			if (this->numObservations <= 0)
			{
				return 0.0;
			}

			// NOTE: The weighted sizes are counted in whole bytes, since a fraction of
			// a byte is not something that was sent.
			int64_t totalBytes{ 0 };
			int64_t lostBytes{ 0 };
			double minLossRate{ 1.0 };
			double maxLossRate{ 0.0 };
			int64_t minLostBytes{ 0 };
			int64_t maxLostBytes{ 0 };
			int64_t minBytesReceived{ 0 };
			int64_t maxBytesReceived{ 0 };
			int64_t sendingRateOfMaxLossObservation{ 0 };

			for (const auto& observation : this->observations)
			{
				if (!observation.IsInitialized())
				{
					continue;
				}

				const double temporalWeight =
				  this->immediateUpperBoundTemporalWeights[(this->numObservations - 1) - observation.id];

				totalBytes += std::llround(temporalWeight * static_cast<double>(observation.sizeBytes));
				lostBytes += std::llround(temporalWeight * static_cast<double>(observation.lostSizeBytes));

				const double lossRate = observation.sizeBytes != 0
				                          ? static_cast<double>(observation.lostSizeBytes) /
				                              static_cast<double>(observation.sizeBytes)
				                          : 0.0;

				// The highest and the lowest are left out only once there are enough
				// observations for that to still leave something.
				if (this->numObservations <= 3)
				{
					continue;
				}

				if (lossRate > maxLossRate)
				{
					maxLossRate = lossRate;
					maxLostBytes =
					  std::llround(temporalWeight * static_cast<double>(observation.lostSizeBytes));
					maxBytesReceived =
					  std::llround(temporalWeight * static_cast<double>(observation.sizeBytes));
					sendingRateOfMaxLossObservation = observation.sendingRate;
				}

				if (lossRate < minLossRate)
				{
					minLossRate = lossRate;
					minLostBytes =
					  std::llround(temporalWeight * static_cast<double>(observation.lostSizeBytes));
					minBytesReceived =
					  std::llround(temporalWeight * static_cast<double>(observation.sizeBytes));
				}
			}

			// NOTE: Nothing was sent at all, so there is no loss to tell. Without this
			// the division below would give a NaN, which makes every comparison it
			// takes part in false and silently freezes the estimate for good.
			if (totalBytes == 0)
			{
				return 0.0;
			}

			// A sudden jump of the sending rate explains the loss of that observation
			// on its own, so it's not a spike to be filtered out.
			if (BitrateUtils::ApplyBitrateFactor(GetMedianSendingRate(), this->options.medianSendingRateFactor) <= sendingRateOfMaxLossObservation)
			{
				return static_cast<double>(lostBytes) / static_cast<double>(totalBytes);
			}

			// It could happen if the window was of two observations.
			if (totalBytes == maxBytesReceived + minBytesReceived)
			{
				return static_cast<double>(lostBytes) / static_cast<double>(totalBytes);
			}

			return static_cast<double>(lostBytes - minLostBytes - maxLostBytes) /
			       static_cast<double>(totalBytes - maxBytesReceived - minBytesReceived);
		}

		int64_t LossBasedController::GetMedianSendingRate() const
		{
			MS_TRACE();

			std::vector<int64_t> sendingRates;

			sendingRates.reserve(this->observations.size());

			for (const auto& observation : this->observations)
			{
				if (!observation.IsInitialized() || observation.sendingRate == 0)
				{
					continue;
				}

				sendingRates.push_back(observation.sendingRate);
			}

			if (sendingRates.empty())
			{
				return 0;
			}

			std::ranges::sort(sendingRates);

			if (sendingRates.size() % 2 == 0)
			{
				return (sendingRates[(sendingRates.size() / 2) - 1] + sendingRates[sendingRates.size() / 2]) /
				       2;
			}

			return sendingRates[sendingRates.size() / 2];
		}

		int64_t LossBasedController::GetImmediateUpperBoundBitrate() const
		{
			MS_TRACE();

			return this->immediateUpperBoundBitrate.value_or(this->maxBitrate);
		}

		void LossBasedController::CalculateImmediateUpperBoundBitrate()
		{
			MS_TRACE();

			int64_t bitrate{ this->maxBitrate };

			// Beyond a bit of loss, the more is observed the less the link is allowed
			// to be told to carry, without waiting for the estimate to converge.
			if (this->averageReportedLossRatio > this->options.immediateUpperBoundLossOffset)
			{
				const double boundBitrate =
				  static_cast<double>(this->options.immediateUpperBoundBitrateBalance) /
				  (this->averageReportedLossRatio - this->options.immediateUpperBoundLossOffset);

				// NOTE: A loss barely above the offset gives a bound beyond what an
				// int64_t can hold, and converting such a value is undefined behaviour.
				// It bounds nothing anyway once it's over the maximum bitrate.
				bitrate = boundBitrate >= static_cast<double>(this->maxBitrate) ? this->maxBitrate
				                                                                : std::llround(boundBitrate);
			}

			this->immediateUpperBoundBitrate = bitrate;
		}

		int64_t LossBasedController::GetImmediateLowerBoundBitrate() const
		{
			MS_TRACE();

			return this->immediateLowerBoundBitrate.value_or(0);
		}

		void LossBasedController::CalculateImmediateLowerBoundBitrate()
		{
			MS_TRACE();

			int64_t bitrate{ 0 };

			// What the link is known to be delivering is never worth going below.
			if (this->acknowledgedBitrate != Types::BitrateInfinite)
			{
				bitrate = BitrateUtils::ApplyBitrateFactor(
				  this->acknowledgedBitrate, this->options.lowerBoundByAckedRateFactor);
			}

			this->immediateLowerBoundBitrate = std::max(bitrate, this->minBitrate);
		}

		void LossBasedController::CalculateTemporalWeights()
		{
			MS_TRACE();

			for (int64_t i{ 0 }; i < this->options.observationWindowSize; ++i)
			{
				this->temporalWeights[i] = std::pow(this->options.temporalWeightFactor, i);
				this->immediateUpperBoundTemporalWeights[i] =
				  std::pow(this->options.immediateUpperBoundTemporalWeightFactor, i);
			}
		}

		bool LossBasedController::IsInLossLimitedState() const
		{
			MS_TRACE();

			return this->result.state != State::DELAY_BASED_ESTIMATE;
		}

		bool LossBasedController::IsEstimateIncreasingWhenLossLimited(
		  int64_t oldEstimate, int64_t newEstimate) const
		{
			MS_TRACE();

			return (oldEstimate < newEstimate ||
			        (oldEstimate == newEstimate && this->result.state == State::INCREASING)) &&
			       IsInLossLimitedState();
		}
	} // namespace BWE
} // namespace RTC
