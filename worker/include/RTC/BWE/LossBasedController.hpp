#ifndef MS_RTC_BWE_LOSS_BASED_CONTROLLER_HPP
#define MS_RTC_BWE_LOSS_BASED_CONTROLLER_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <ankerl/unordered_dense.h>
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Tells the loss caused by congestion from the loss a link has by itself.
		 *
		 * A link can lose packets without being congested at all, which is the normal
		 * state of Wi-Fi and mobile links, and reacting to that loss by sending less
		 * doesn't reduce it: it just gives away capacity that was there. Telling both
		 * apart cannot be done by looking at a loss figure, because the very same
		 * figure means different things at different sending rates.
		 *
		 * So instead of a rule on the reported loss, this is an estimator of two
		 * values that together describe the link: how much it loses when it is not
		 * congested, and how much it can carry. With those two, the loss to expect
		 * from an observation is known: the inherent one while sending below the
		 * capacity, plus the share of the excess while sending above it. The pair of
		 * values that makes the losses actually observed most likely is the estimate,
		 * and the bitrate of that pair is the answer.
		 *
		 * Candidates slightly above, at and below the current estimate are tried on
		 * every update, each refined by a step of Newton's method on the inherent
		 * loss, and the most likely one wins. A bias towards higher bitrates keeps
		 * the estimate from settling low when the observations cannot tell them
		 * apart, and fades away as the observed loss grows.
		 *
		 * @remarks
		 * - This class is a port of the LossBasedBweV2 class in libwebrtc (renamed to
		 *   a better name).
		 */
		class LossBasedController
		{
		public:
			struct LossBasedControllerOptions
			{
				/**
				 * Factors applied to the current estimate to build the candidates that
				 * are tried on every update.
				 */
				std::vector<double> candidateFactors{ 1.02, 1.0, 0.95 };
				/**
				 * How much the estimate may grow over the acknowledged bitrate while
				 * the link is loss limited.
				 */
				double bitrateRampupUpperBoundFactor{ 1.5 };
				/**
				 * The same while the estimate is being held, which is more conservative
				 * because holding means that a higher bitrate already caused loss.
				 */
				double bitrateRampupUpperBoundFactorInHold{ 1.2 };
				/**
				 * How far below the held bitrate the acknowledged one has to be for the
				 * factor above to be the one applied.
				 */
				double bitrateRampupHoldThreshold{ 1.3 };
				/**
				 * How much of the acknowledged bitrate the estimate may grow by on top
				 * of its usual bound, the longer the more time has passed since it was
				 * last reduced. Zero disables that acceleration.
				 */
				double rampupAccelerationMaxFactor{ 0.0 };
				/**
				 * Time since the last reduction at which that acceleration is at its
				 * fullest.
				 */
				int64_t rampupAccelerationMaxoutTimeUs{ 60 * 1000 * 1000 };
				/**
				 * Weight given to a higher bitrate when choosing among candidates, so
				 * that observations which cannot tell them apart don't settle low.
				 */
				double higherBitrateBiasFactor{ 0.0002 };
				/**
				 * The same, applied to the logarithm of the bitrate, so that the bias
				 * doesn't grow without bound.
				 */
				double higherLogBitrateBiasFactor{ 0.02 };
				/**
				 * Observed loss at which the bias above is gone, since at that much loss
				 * preferring a higher bitrate is not defensible anymore.
				 */
				double lossThresholdOfHighBitratePreference{ 0.2 };
				/**
				 * How abruptly that bias fades as the observed loss approaches the
				 * threshold above.
				 */
				double bitratePreferenceSmoothingFactor{ 0.002 };
				/**
				 * Lowest inherent loss that may be estimated for a link.
				 */
				double inherentLossLowerBound{ 1.0e-3 };
				/**
				 * Inherent loss that may be estimated at an unbounded bitrate, which
				 * grows as the bitrate gets lower by the balance below.
				 */
				double inherentLossUpperBoundOffset{ 0.05 };
				/**
				 * Bitrate at which the inherent loss upper bound grows by one, so that a
				 * low bitrate is allowed to be explained by a lossy link.
				 */
				int64_t inherentLossUpperBoundBitrateBalance{ 100000 };
				/**
				 * Inherent loss assumed before anything has been observed.
				 */
				double initialInherentLossEstimate{ 0.01 };
				/**
				 * Steps of Newton's method applied to each candidate.
				 */
				int64_t newtonIterations{ 1 };
				/**
				 * Fraction of the step that each of those iterations takes.
				 */
				double newtonStepSize{ 0.75 };
				/**
				 * Whether the acknowledged bitrate is tried as a candidate of its own.
				 */
				bool appendAcknowledgedRateCandidate{ true };
				/**
				 * Whether the delay based estimate is tried as a candidate of its own
				 * while it's above the current one.
				 */
				bool appendDelayBasedEstimateCandidate{ true };
				/**
				 * Whether the bound that the observed loss puts on the estimate is tried
				 * as a candidate while not filling the link, which lets the estimate
				 * come down to it in one step instead of gradually.
				 */
				bool appendUpperBoundCandidateInAlr{ false };
				/**
				 * Shortest span of send times that an observation may cover.
				 */
				int64_t observationDurationLowerBoundUs{ 250 * 1000 };
				/**
				 * Observations kept, which is how far back the estimate looks.
				 */
				int64_t observationWindowSize{ 15 };
				/**
				 * Observations needed before this controller may be used at all.
				 */
				int64_t minNumObservations{ 3 };
				/**
				 * How much the sending rate of an observation is smoothed with the one
				 * of the previous observation.
				 *
				 * @remarks
				 * - Zero means no smoothing at all, which is what libwebrtc does. A
				 *   sender of a couple of streams has a sending rate that only changes
				 *   when it decides so, while ours is the sum of many streams whose
				 *   makeup changes on its own, and those changes are not the network.
				 */
				double sendingRateSmoothingFactor{ 0.0 };
				/**
				 * How much weight each observation loses per observation of age when
				 * looking for the most likely pair of values.
				 */
				double temporalWeightFactor{ 0.9 };
				/**
				 * The same, for the average observed loss that bounds the estimate right
				 * away.
				 */
				double immediateUpperBoundTemporalWeightFactor{ 0.9 };
				/**
				 * Observed loss under which no immediate upper bound is applied.
				 */
				double immediateUpperBoundLossOffset{ 0.05 };
				/**
				 * Bitrate the immediate upper bound allows per unit of observed loss
				 * over the offset above.
				 */
				int64_t immediateUpperBoundBitrateBalance{ 100000 };
				/**
				 * How much of the acknowledged bitrate the estimate may never go below.
				 */
				double lowerBoundByAckedRateFactor{ 1.0 };
				/**
				 * Fraction of the acknowledged bitrate taken as a candidate when backing
				 * off.
				 */
				double bitrateBackoffLowerBoundFactor{ 1.0 };
				/**
				 * How much the estimate may grow within the window that follows a
				 * decrease.
				 */
				double maxIncreaseFactor{ 1.3 };
				/**
				 * How long that window lasts.
				 */
				int64_t delayedIncreaseWindowUs{ 300 * 1000 };
				/**
				 * Whether the estimate is held back while the loss observed is worse
				 * than the one the chosen candidate says the link has by itself, since
				 * then the excess is ours to fix.
				 */
				bool notIncreaseIfInherentLossLessThanAverageLoss{ true };
				/**
				 * Whether the acknowledged bitrate stops being a candidate while not
				 * filling the link, since then what it delivers says nothing about what
				 * it could deliver.
				 */
				bool notUseAckedRateInAlr{ true };
				/**
				 * Whether this controller may take the estimate over while the target is
				 * still in its start phase.
				 */
				bool useInStartPhase{ true };
				/**
				 * How much each hold lasts compared to the previous one, which makes
				 * repeated failures to increase back off for longer.
				 */
				double holdDurationFactor{ 2.0 };
				/**
				 * Whether loss is measured in bytes rather than in packets, which tells
				 * a lost packet of a full frame apart from a lost one carrying almost
				 * nothing.
				 */
				bool useByteLossRate{ true };
				/**
				 * Whether an estimate that had to be brought down to its bounds stops
				 * being kept as a description of the link.
				 */
				bool boundBestCandidate{ true };
				/**
				 * How much the sending rate of an observation has to exceed the median
				 * of the window for its loss to be taken at face value instead of as a
				 * spike.
				 */
				double medianSendingRateFactor{ 2.0 };
			};

			/**
			 * What this controller is doing with the estimate, which the orchestrator
			 * needs in order to decide whether to probe.
			 */
			enum class State : uint8_t
			{
				/**
				 * The link is loss limited and the estimate is growing back.
				 */
				INCREASING,
				/**
				 * The link is loss limited and the estimate is being reduced.
				 */
				DECREASING,
				/**
				 * The link is not loss limited, so the delay based estimate rules.
				 */
				DELAY_BASED_ESTIMATE
			};

			struct Result
			{
				int64_t bitrate{ 0 };
				State state{ State::DELAY_BASED_ESTIMATE };
			};

		private:
			/**
			 * The pair of values that describe a link.
			 */
			struct ChannelParameters
			{
				/**
				 * Loss the link has while not congested at all.
				 */
				double inherentLoss{ 0.0 };
				/**
				 * Bitrate the link can carry, or zero while it's not known yet.
				 */
				int64_t lossLimitedBitrate{ 0 };
			};

			/**
			 * First and second derivatives of the likelihood with respect to the
			 * inherent loss, which is what Newton's method needs.
			 */
			struct Derivatives
			{
				double first{ 0.0 };
				double second{ 0.0 };
			};

			/**
			 * What was sent and lost over a span of send times, which is the unit this
			 * controller reasons about.
			 */
			struct Observation
			{
				bool IsInitialized() const
				{
					return this->id != -1;
				}

				int64_t numPackets{ 0 };
				int64_t numLostPackets{ 0 };
				int64_t numReceivedPackets{ 0 };
				int64_t sendingRate{ 0 };
				int64_t sizeBytes{ 0 };
				int64_t lostSizeBytes{ 0 };
				int64_t id{ -1 };
			};

			/**
			 * Feedback accumulated so far, which becomes an observation once it covers
			 * a long enough span of send times.
			 */
			struct PartialObservation
			{
				/**
				 * Size of the packets reported as lost, by sequence number, so that a
				 * packet reported lost and received later stops counting.
				 */
				ankerl::unordered_dense::map<int64_t, int64_t> lostPackets;
				int64_t numPackets{ 0 };
				int64_t sizeBytes{ 0 };
			};

			/**
			 * The bitrate the estimate is not allowed to grow above while holding, and
			 * for how long.
			 */
			struct HoldInfo
			{
				int64_t atUs{ 0 };
				int64_t durationUs{ 0 };
				int64_t bitrate{ Types::BitrateInfinite };
			};

		public:
			LossBasedController();

			explicit LossBasedController(LossBasedControllerOptions options);

			/**
			 * Whether enough has been observed for this controller to say anything.
			 */
			bool IsReady() const;

			/**
			 * Whether this controller may take the estimate over while the target is
			 * still in its start phase.
			 */
			bool IsReadyToUseInStartPhase() const;

			/**
			 * Latest estimate, or the delay based one while this controller cannot say
			 * anything yet.
			 */
			Result GetResult() const;

			void SetAcknowledgedBitrate(int64_t acknowledgedBitrate);

			void SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate);

			/**
			 * Place the estimate at the given bitrate.
			 */
			void SetBitrateEstimate(int64_t bitrate);

			/**
			 * Feed the results of a feedback message.
			 *
			 * @param delayBasedEstimate - Estimate of the delay based path, which
			 * bounds this one, or `Types::BitrateInfinite` if there is none.
			 * @param inAlr - Whether the sender is not sending enough to fill the link,
			 * in which case the acknowledged bitrate says nothing about its capacity.
			 */
			void UpdateBitrateEstimate(
			  const std::vector<Types::PacketResult>& packetResults, int64_t delayBasedEstimate, bool inAlr);

		private:
			/**
			 * Add the given results to the observation being accumulated, and close it
			 * if it already covers a long enough span of send times.
			 *
			 * @returns Whether an observation was closed, which is when the estimate
			 * can be recalculated.
			 */
			bool PushBackObservation(const std::vector<Types::PacketResult>& packetResults);

			/**
			 * Candidates to try on this update, each already within bounds.
			 */
			std::vector<ChannelParameters> GetCandidates(bool inAlr) const;

			/**
			 * Highest bitrate a candidate may have.
			 */
			int64_t GetCandidateBitrateUpperBound() const;

			/**
			 * How likely the losses observed are if the link were as the given pair of
			 * values says, plus the bias towards higher bitrates.
			 */
			double GetObjective(const ChannelParameters& channelParameters) const;

			Derivatives GetDerivatives(const ChannelParameters& channelParameters) const;

			/**
			 * Refine the inherent loss of the given candidate.
			 */
			void ApplyNewtonsMethod(ChannelParameters& channelParameters) const;

			/**
			 * Bring the inherent loss of the given candidate within what a link at its
			 * bitrate could plausibly have.
			 */
			double GetFeasibleInherentLoss(const ChannelParameters& channelParameters) const;

			double GetInherentLossUpperBound(int64_t bitrate) const;

			double AdjustBiasFactor(double lossRate, double biasFactor) const;

			double GetHighBitrateBias(int64_t bitrate) const;

			/**
			 * Sending rate an observation is taken to have, which is its own smoothed
			 * with the one of the previous observation.
			 */
			int64_t GetSendingRate(int64_t instantSendingRate) const;

			/**
			 * Average loss observed over the window, leaving out the highest and the
			 * lowest so that a single spike doesn't drag it.
			 */
			void UpdateAverageReportedLossRatio();

			double CalculateAverageReportedPacketLossRatio() const;

			/**
			 * The same by bytes, which tells a lost packet carrying a full frame apart
			 * from one carrying almost nothing.
			 */
			double CalculateAverageReportedByteLossRatio() const;

			int64_t GetMedianSendingRate() const;

			/**
			 * Bound that the observed loss puts on the estimate right away, without
			 * waiting for it to converge.
			 */
			int64_t GetImmediateUpperBoundBitrate() const;

			void CalculateImmediateUpperBoundBitrate();

			int64_t GetImmediateLowerBoundBitrate() const;

			void CalculateImmediateLowerBoundBitrate();

			void CalculateTemporalWeights();

			/**
			 * Whether the link is loss limited at all, which is what tells this
			 * controller's estimate apart from just following the delay based one.
			 */
			bool IsInLossLimitedState() const;

			bool IsEstimateIncreasingWhenLossLimited(int64_t oldEstimate, int64_t newEstimate) const;

		private:
			// Passed by argument.
			const LossBasedControllerOptions options;
			// Others.
			// Observations of the window, indexed by their id modulo its size.
			std::vector<Observation> observations;
			// Weight of an observation per observation of age.
			std::vector<double> temporalWeights;
			// The same, for the average observed loss.
			std::vector<double> immediateUpperBoundTemporalWeights;
			struct PartialObservation partialObservation;
			struct ChannelParameters currentBestEstimate;
			struct HoldInfo lastHoldInfo;
			struct Result result;
			// Observations closed so far, which never decreases and hence also gives
			// the id of the next one.
			int64_t numObservations{ 0 };
			// Highest send time of the latest observation, which is the clock this
			// controller runs on. Feedback is the only thing that moves it.
			std::optional<int64_t> lastSendTimeOfLatestObservationUs;
			double averageReportedLossRatio{ 0.0 };
			int64_t acknowledgedBitrate{ Types::BitrateInfinite };
			int64_t delayBasedEstimate{ Types::BitrateInfinite };
			int64_t minBitrate{ 0 };
			int64_t maxBitrate{ Types::BitrateInfinite };
			std::optional<int64_t> immediateUpperBoundBitrate;
			std::optional<int64_t> immediateLowerBoundBitrate;
			// Bitrate the estimate may not grow above until the window that follows a
			// decrease is over.
			int64_t bitrateLimitInCurrentWindow{ Types::BitrateInfinite };
			std::optional<int64_t> recoveringAfterLossAtUs;
			// Instant at which the estimate was last brought down, which is what the
			// rampup acceleration grows from.
			std::optional<int64_t> lastBitrateReducedAtUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
