#ifndef MS_RTC_BWE_TARGET_RATE_CONTROLLER_HPP
#define MS_RTC_BWE_TARGET_RATE_CONTROLLER_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/LossBasedController.hpp"
#include <deque>
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Holds the target bitrate and every bound that applies to it.
		 *
		 * The delay based path, the receiver of the media and the application each
		 * produce a limit rather than a bitrate, so something has to hold the value
		 * they bound and decide how it moves within them. That is this class, and it
		 * also owns the two inputs that move the target down on their own: a round
		 * trip time long enough to mean that the queues of the network are full, and
		 * the loss the receiver reports.
		 *
		 * How loss moves the target depends on how much is known about it. Until the
		 * loss controller has seen enough feedback to tell congestion from the loss a
		 * link has by itself, this class applies rules of its own on the fraction of
		 * packets reported as lost: increase while it stays under 2%, leave the target
		 * alone up to 10%, and decrease it above that. They are coarse on purpose, and
		 * their weakness is precisely what they cannot tell apart, so on a link with
		 * steady loss above 2% they hold the target still and above 10% they drive it
		 * to the minimum. As soon as the loss controller is ready, its estimate
		 * replaces them.
		 *
		 * @remarks
		 * - This class is a port of the SendSideBandwidthEstimation class in
		 *   libwebrtc (renamed to a better name).
		 */
		class TargetRateController
		{
		public:
			struct TargetRateControllerOptions
			{
				/**
				 * Reported loss under which the bitrate is increased.
				 */
				double lowLossThreshold{ 0.02 };
				/**
				 * Reported loss over which the bitrate is decreased.
				 */
				double highLossThreshold{ 0.1 };
				/**
				 * Bitrate under which loss is ignored altogether, since at a low enough
				 * bitrate loss is unlikely to be caused by the traffic itself.
				 */
				int64_t bitrateThreshold{ 0 };
				/**
				 * Whether the bound asked for by the receiver takes part in how the
				 * target moves, rather than only capping the value this controller
				 * hands out.
				 */
				bool disableReceiverLimitCapsOnly{ false };
				/**
				 * Round trip time over which the bitrate is dropped regardless of loss.
				 * A round trip time this long means the queues of the network are full.
				 */
				int64_t maxRttUs{ 3 * 1000 * 1000 };
				/**
				 * Fraction of the target the bitrate is dropped to on each of those
				 * drops.
				 */
				double rttBackoffDropFraction{ 0.8 };
				/**
				 * How often the bitrate may be dropped due to the round trip time.
				 */
				int64_t rttBackoffDropIntervalUs{ 1000 * 1000 };
				/**
				 * Bitrate the round trip time backoff never drops below.
				 */
				int64_t rttBackoffBitrateFloor{ 5000 };
			};

		private:
			/**
			 * Drops the bitrate when the round trip time stays long enough to mean
			 * that the queues of the network are full, which is a congestion signal
			 * that loss and delay may both miss.
			 */
			struct RttBackoff
			{
				/**
				 * Round trip time corrected for the sender being idle. Without the
				 * correction a sender that stops transmitting would keep reporting the
				 * last measured value forever.
				 */
				int64_t GetCorrectedRttUs() const
				{
					int64_t timeoutCorrectionUs{ 0 };

					// While either instant is missing there is nothing to correct: no
					// feedback has arrived, or nothing that gets feedback has been sent.
					if (this->lastPacketSentAtUs.has_value() && this->lastPropagationRttAtUs.has_value())
					{
						timeoutCorrectionUs = std::max<int64_t>(
						  this->lastPacketSentAtUs.value() - this->lastPropagationRttAtUs.value(), 0);
					}

					return timeoutCorrectionUs + this->lastPropagationRttUs;
				}

				std::optional<int64_t> lastPropagationRttAtUs;
				std::optional<int64_t> lastPacketSentAtUs;
				int64_t lastPropagationRttUs{ 0 };
			};

		public:
			TargetRateController();

			explicit TargetRateController(TargetRateControllerOptions options);

			/**
			 * Bitrate this controller may produce, already within every bound.
			 */
			int64_t GetTargetBitrate() const;

			/**
			 * Latest fraction of lost packets reported by the receiver, in 1/256 units.
			 */
			uint8_t GetFractionLost() const
			{
				return this->lastFractionLost;
			}

			int64_t GetRttUs() const
			{
				return this->lastRttUs;
			}

			/**
			 * Whether the round trip time is long enough to be taken as congestion on
			 * its own.
			 */
			bool IsRttAboveLimit() const
			{
				return this->rttBackoff.GetCorrectedRttUs() > this->options.maxRttUs;
			}

			/**
			 * Forget everything learnt about the network, which is what a transport
			 * that is not going through the same path anymore calls for.
			 *
			 * @remarks
			 * - The bounds set by the application are dropped too, so they have to be
			 *   set again afterwards.
			 */
			void OnRouteChange();

			/**
			 * Bounds set by the application.
			 *
			 * @remarks
			 * - A max bitrate of zero means no limit.
			 */
			void SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate);

			/**
			 * Place the target at the given bitrate, dropping the bound that could
			 * hold it back and the history that limits how fast it grows.
			 */
			void SetSendBitrate(int64_t bitrate);

			/**
			 * Upper bound produced by the delay based path.
			 */
			void SetDelayBasedEstimate(int64_t bitrate);

			/**
			 * Upper bound asked for by the receiver of the media, which reaches us as
			 * a REMB message.
			 *
			 * @remarks
			 * - A bitrate of zero means no limit.
			 */
			void SetReceiverEstimate(int64_t bitrate);

			/**
			 * Feed a loss report of the receiver.
			 *
			 * @remarks
			 * - Reports are accumulated until they cover enough packets, so most calls
			 *   change nothing.
			 */
			void UpdatePacketsLost(int64_t lostPackets, int64_t totalPackets, int64_t nowUs);

			void UpdateRtt(int64_t rttUs);

			/**
			 * Feed the round trip time the round trip time backoff watches, which is
			 * the one measured by the transport feedback rather than by RTCP.
			 */
			void UpdatePropagationRtt(int64_t propagationRttUs, int64_t nowUs);

			/**
			 * Tell that a packet that will be reported back has just been sent.
			 */
			void OnPacketSent(int64_t sentAtUs);

			/**
			 * Reconsider the target. Called after every loss report and periodically,
			 * since the increase depends on how much time has elapsed.
			 */
			void Update(int64_t nowUs);

			/**
			 * Bitrate the network is known to be delivering.
			 */
			void SetAcknowledgedBitrate(int64_t acknowledgedBitrate);

			/**
			 * Feed the results of a feedback message to the loss controller, which is
			 * the only thing in here that looks at packets one by one.
			 *
			 * @param inAlr - Whether the sender is not sending enough to fill the link.
			 */
			void UpdateLossBasedController(
			  const std::vector<Types::PacketResult>& packetResults, bool inAlr, int64_t nowUs);

			/**
			 * What the loss controller is doing with the target, which tells whether
			 * probing makes sense.
			 */
			LossBasedController::State GetLossBasedState() const
			{
				return this->lossBasedState;
			}

		private:
			/**
			 * Whether the target is still allowed to just follow the bounds because
			 * not enough is known about the network yet.
			 */
			bool IsInStartPhase(int64_t nowUs) const;

			/**
			 * Lowest bitrate the target has had during the last second, which is what
			 * the increase grows from.
			 */
			void UpdateMinBitrateHistory(int64_t nowUs);

			/**
			 * Upper bound of every kind that applies to the target.
			 */
			int64_t GetUpperLimit() const;

			/**
			 * Set the target to the given bitrate, keeping it within its bounds.
			 */
			void SetTargetBitrate(int64_t bitrate);

			/**
			 * Reapply the bounds to the current target, which is needed when one of
			 * them changes.
			 */
			void ApplyTargetLimits();

		private:
			// Passed by argument.
			const TargetRateControllerOptions options;
			// Others.
			// Tells congestion from the loss the link has by itself, and takes over
			// the target as soon as it has enough observations.
			LossBasedController lossBasedController;
			LossBasedController::State lossBasedState{ LossBasedController::State::DELAY_BASED_ESTIMATE };
			struct RttBackoff rttBackoff;
			// Lowest target of the last second, as a sliding window minimum of
			// (instant in us, bitrate in bps) pairs.
			std::deque<std::pair<int64_t, int64_t>> minBitrateHistory;
			// Loss reports too small to tell anything on their own, accumulated until
			// they cover enough packets.
			int64_t lostPacketsSinceLastLossUpdate{ 0 };
			int64_t expectedPacketsSinceLastLossUpdate{ 0 };
			int64_t currentTarget{ 0 };
			int64_t minBitrateConfigured{ 0 };
			int64_t maxBitrateConfigured{ 0 };
			// Instant of the first loss report ever received, which is when the start
			// phase begins.
			std::optional<int64_t> firstLossReportAtUs;
			// Instant of the latest loss report that covered enough packets to tell
			// something.
			std::optional<int64_t> lastLossReportAtUs;
			// Instant at which the target was last decreased, by the loss rules or by
			// the round trip time backoff.
			std::optional<int64_t> lastDecreaseAtUs;
			// Whether the target has already been decreased for the loss currently
			// reported, so that a same report doesn't decrease it twice.
			bool hasDecreasedSinceLastFractionLoss{ false };
			uint8_t lastFractionLost{ 0 };
			int64_t lastRttUs{ 0 };
			// Upper bound asked for by the receiver of the media.
			int64_t receiverLimit{ Types::BitrateInfinite };
			// Upper bound produced by the delay based path.
			int64_t delayBasedLimit{ Types::BitrateInfinite };
		};
	} // namespace BWE
} // namespace RTC

#endif
