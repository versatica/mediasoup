#ifndef MS_RTC_BWE_PROBE_CONTROLLER_HPP
#define MS_RTC_BWE_PROBE_CONTROLLER_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Decides when to probe and at what bitrate.
		 *
		 * Probing is the only way to discover capacity that is not being used,
		 * because nothing measured while sending below the link's capacity says
		 * anything about how much more it could carry. So this asks for bursts: at
		 * the start of a connection to climb fast instead of waiting for the slow
		 * ramp, periodically while the sender isn't filling the link, when the
		 * application asks for more than is being sent, and after a large drop to
		 * check whether it was real.
		 *
		 * It decides nothing else: every method returns the bursts to launch, and
		 * emitting them is somebody else's job. And it refuses to probe whenever
		 * something is already known to be holding the estimate back, since a burst
		 * then measures that limit rather than the link.
		 */
		class ProbeController
		{
		public:
			/**
			 * Why the estimate is not free to grow, which decides whether probing
			 * makes sense at all.
			 */
			enum class BandwidthLimitedCause : uint8_t
			{
				/**
				 * Loss is what limits the estimate, and it's growing back.
				 */
				LOSS_LIMITED_BWE_INCREASING = 0,
				/**
				 * Loss is what limits the estimate and it isn't growing.
				 */
				LOSS_LIMITED_BWE,
				/**
				 * The delay based path is what limits the estimate.
				 */
				DELAY_BASED_LIMITED,
				/**
				 * The same, with the delay growing right now.
				 */
				DELAY_BASED_LIMITED_DELAY_INCREASED,
				/**
				 * A round trip time long enough to mean that the queues are full.
				 */
				RTT_BASED_BACK_OFF_HIGH_RTT
			};

			/**
			 * @remarks
			 * - Every constraint documented below is checked by the constructor and
			 *   aborts when it doesn't hold. These options are set from C++ alone,
			 *   never from the network nor from the API, so breaking one of them is a
			 *   programming error.
			 */
			struct ProbeControllerOptions
			{
				/**
				 * Factor applied to the start bitrate for the first burst of the
				 * connection.
				 */
				double firstExponentialProbeScale{ 3.0 };
				/**
				 * The same for the second one, or no value to send just one.
				 */
				std::optional<double> secondExponentialProbeScale{ 6.0 };
				/**
				 * Factor applied to the measured bitrate for each burst that follows one
				 * whose result was good enough to keep climbing.
				 */
				double furtherExponentialProbeScale{ 2.0 };
				/**
				 * Fraction of the bitrate a burst was sent at that its result has to
				 * exceed for another one to follow it.
				 */
				double furtherProbeThreshold{ 0.7 };
				/**
				 * Whether to stop climbing as soon as the measured bitrate is already
				 * above what the application asked for.
				 */
				bool abortFurtherProbeIfMaxLowerThanCurrent{ false };
				/**
				 * How long after the first burst the initial ones keep being repeated,
				 * while repeated initial probing is enabled.
				 */
				int64_t repeatedInitialProbingTimePeriodUs{ 5 * 1000 * 1000 };
				/**
				 * How long each of those initial bursts lasts. It's longer than a usual
				 * one because there is no traffic yet to measure against.
				 */
				int64_t initialProbeDurationUs{ 100 * 1000 };
				/**
				 * Time between two consecutive bursts of packets within one of those
				 * initial probes.
				 *
				 * @remarks
				 * - It cannot be zero, since it's what the bitrate of a burst is held
				 *   over to decide how many bytes each of its shots carries.
				 */
				int64_t initialMinProbeDeltaUs{ 20 * 1000 };
				/**
				 * How often to probe while the sender is not filling the link.
				 */
				int64_t alrProbingIntervalUs{ 5 * 1000 * 1000 };
				/**
				 * Factor applied to the estimate for those bursts.
				 */
				double alrProbeScale{ 2.0 };
				/**
				 * How often to probe while an upper bound of the link capacity is known,
				 * or no limit at all to never do it, which is what turns that whole path
				 * off.
				 */
				int64_t networkStateEstimateProbingIntervalUs{ Types::TimeUsInfinite };
				/**
				 * Fraction of that upper bound below which the estimate is low enough to
				 * keep probing periodically.
				 */
				double probeIfEstimateLowerThanNetworkStateEstimateRatio{ 0.0 };
				/**
				 * How often to probe while the estimate is that low.
				 */
				int64_t estimateLowerThanNetworkStateEstimateProbingIntervalUs{ 3 * 1000 * 1000 };
				/**
				 * Factor applied to that upper bound when it's what bounds the burst.
				 */
				double networkStateProbeScale{ 1.0 };
				/**
				 * How long a burst lasts when that upper bound is known and is above
				 * what the burst aims for.
				 */
				int64_t networkStateProbeDurationUs{ 15 * 1000 };
				/**
				 * Time between two consecutive bursts of packets in that case.
				 *
				 * @remarks
				 * - It cannot be zero, since it's what the bitrate of a burst is held
				 *   over to decide how many bytes each of its shots carries.
				 */
				int64_t networkStateMinProbeDeltaUs{ 20 * 1000 };
				/**
				 * Whether to probe when the application raises how much it wants to
				 * send.
				 */
				bool probeOnMaxAllocatedBitrateChange{ true };
				/**
				 * Whether to do it also while the sender is filling the link.
				 */
				bool probeOnMaxAllocatedBitrateChangeWithoutAlr{ true };
				/**
				 * Factor applied to what the application asks for, for the first of
				 * those bursts, or no value to send none.
				 */
				std::optional<double> firstAllocationProbeScale{ 1.0 };
				/**
				 * The same for the second one, or no value to send just one.
				 */
				std::optional<double> secondAllocationProbeScale{ 2.0 };
				/**
				 * Factor applied to the current estimate, which is how far above it
				 * those bursts are allowed to aim.
				 */
				double allocationProbeLimitByCurrentScale{ 2.0 };
				/**
				 * Packets a burst is meant to be made of.
				 */
				int64_t minProbePacketsSent{ 5 };
				/**
				 * How long a burst lasts.
				 */
				int64_t minProbeDurationUs{ 15 * 1000 };
				/**
				 * Time between two consecutive bursts of packets within a probe.
				 *
				 * @remarks
				 * - It cannot be zero, since it's what the bitrate of a burst is held
				 *   over to decide how many bytes each of its shots carries.
				 */
				int64_t minProbeDeltaUs{ 2 * 1000 };
				/**
				 * Factor applied to the estimate that bounds a burst while loss is what
				 * limits it and it's growing back.
				 */
				double lossLimitedProbeScale{ 1.5 };
				/**
				 * Fraction of the maximum above which the estimate is already high
				 * enough for a burst to be pointless. Zero never skips one.
				 */
				double skipIfEstimateLargerThanFractionOfMax{ 0.0 };
				/**
				 * Factor applied to what the application asks for when deciding that.
				 */
				double skipProbeMaxAllocatedScale{ 1.0 };
				/**
				 * Whether a large drop may be probed even while the sender is filling
				 * the link.
				 */
				bool rapidRecoveryExperiment{ false };
			};

		private:
			/**
			 * Where the climbing that follows the first burst of a connection is.
			 */
			enum class State : uint8_t
			{
				/**
				 * Nothing has been probed yet.
				 */
				INIT,
				/**
				 * A burst went out and its result decides whether another one follows.
				 */
				WAITING_FOR_PROBING_RESULT,
				/**
				 * There is nothing left to climb.
				 */
				PROBING_COMPLETE
			};

		public:
			ProbeController();

			explicit ProbeController(ProbeControllerOptions options);

			/**
			 * Bounds set by the application, plus the bitrate to start from.
			 *
			 * @param maxBitrate - `Types::BitrateInfinite` for no limit, in which case
			 * the highest bitrate that may be probed applies instead.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> SetBitrates(
			  int64_t minBitrate, int64_t startBitrate, int64_t maxBitrate, int64_t nowUs);

			/**
			 * Total of what the application wants to send, which is not the same as
			 * the maximum it allows: it's the sum over the streams being sent.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> OnMaxTotalAllocatedBitrate(
			  int64_t maxTotalAllocatedBitrate, int64_t nowUs);

			[[nodiscard]] std::vector<Types::ProbeClusterConfig> OnNetworkAvailability(
			  bool networkAvailable, int64_t nowUs);

			/**
			 * Feed the current estimate and what is holding it back.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> SetEstimatedBitrate(
			  int64_t bitrate, BandwidthLimitedCause bandwidthLimitedCause, int64_t nowUs);

			void EnablePeriodicAlrProbing(bool enable)
			{
				this->enablePeriodicAlrProbing = enable;
			}

			/**
			 * Whether the initial bursts keep being repeated for a while, which only
			 * makes sense until there is traffic of its own to measure.
			 */
			void EnableRepeatedInitialProbing(bool enable)
			{
				this->repeatedInitialProbingEnabled = enable;
			}

			void SetAlrStartTimeUs(std::optional<int64_t> alrStartTimeUs)
			{
				this->alrStartTimeUs = alrStartTimeUs;
			}

			void SetAlrEndedTimeUs(int64_t alrEndedTimeUs)
			{
				this->alrEndedTimeUs = alrEndedTimeUs;
			}

			/**
			 * Ask for a burst after the estimate has come back up from a large drop,
			 * to tell a real drop from a passing one.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> RequestProbe(int64_t nowUs);

			/**
			 * Upper bound of the link capacity as measured elsewhere (bps), which
			 * bounds what a burst may aim for.
			 *
			 * @remarks
			 * - `Types::BitrateInfinite` means that nothing measured it, which is what
			 *   the link capacity estimator answers while it has no estimate, and is
			 *   kept as no bound at all rather than as an enormous one.
			 */
			void SetLinkCapacityUpperBound(int64_t linkCapacityUpperBound)
			{
				this->linkCapacityUpperBound = linkCapacityUpperBound != Types::BitrateInfinite
				                                 ? std::optional<int64_t>(linkCapacityUpperBound)
				                                 : std::nullopt;
			}

			/**
			 * Forget what was probed, leaving the settings alone, so that the climbing
			 * of a new connection starts over.
			 */
			void Reset(int64_t nowUs);

			/**
			 * Reconsider whether a burst is due, which is what drives the periodic
			 * ones.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> Process(int64_t nowUs);

		private:
			void UpdateState(State newState);

			[[nodiscard]] std::vector<Types::ProbeClusterConfig> InitiateExponentialProbing(int64_t nowUs);

			/**
			 * Turn the given bitrates into bursts, bounding them by everything that
			 * applies and refusing them altogether while something is known to be
			 * holding the estimate back.
			 *
			 * @param probeFurther - Whether the result of these bursts may lead to
			 * another one.
			 */
			[[nodiscard]] std::vector<Types::ProbeClusterConfig> InitiateProbing(
			  int64_t nowUs, const std::vector<int64_t>& bitratesToProbe, bool probeFurther);

			bool IsTimeForAlrProbe(int64_t nowUs) const;

			bool IsTimeForNetworkStateProbe(int64_t nowUs) const;

			bool IsTimeForNextRepeatedInitialProbe(int64_t nowUs) const;

			Types::ProbeClusterConfig CreateProbeClusterConfig(int64_t nowUs, int64_t bitrate);

		private:
			// Passed by argument.
			const ProbeControllerOptions options;
			// Others.
			State state{ State::INIT };
			bool networkAvailable{ false };
			bool repeatedInitialProbingEnabled{ false };
			bool enablePeriodicAlrProbing{ false };
			BandwidthLimitedCause bandwidthLimitedCause{ BandwidthLimitedCause::DELAY_BASED_LIMITED };
			// Instant until which the initial bursts keep being repeated.
			std::optional<int64_t> lastAllowedRepeatedInitialProbeAtUs;
			// Bitrate a burst has to beat for another one to follow it, or
			// `Types::BitrateInfinite` while nothing is to follow.
			int64_t minBitrateToProbeFurther{ Types::BitrateInfinite };
			std::optional<int64_t> lastProbingInitiatedAtUs;
			int64_t estimatedBitrate{ 0 };
			// Upper bound of the link capacity as measured elsewhere, or no value
			// while nothing measured it.
			std::optional<int64_t> linkCapacityUpperBound;
			int64_t startBitrate{ 0 };
			int64_t maxBitrate{ Types::BitrateInfinite };
			std::optional<int64_t> lastBweDropProbingAtUs;
			std::optional<int64_t> alrStartTimeUs;
			std::optional<int64_t> alrEndedTimeUs;
			// Instant of the latest large drop of the estimate, and what it was worth
			// just before it.
			std::optional<int64_t> lastLargeDropAtUs;
			int64_t bitrateBeforeLastLargeDrop{ 0 };
			// Total of what the application wants to send.
			int64_t maxTotalAllocatedBitrate{ 0 };
			// Id given to the next burst, which never repeats.
			int64_t nextProbeClusterId{ 1 };
		};
	} // namespace BWE
} // namespace RTC

#endif
