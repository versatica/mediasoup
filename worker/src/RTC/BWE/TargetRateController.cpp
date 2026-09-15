#define MS_CLASS "RTC::BWE::TargetRateController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/TargetRateController.hpp"
#include "Logger.hpp"
#include "RTC/BWE/Utils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How far back the lowest target is looked for when increasing.
		static constexpr int64_t BweIncreaseIntervalUs{ 1000 * 1000 };
		// Minimum time between two decreases caused by loss, on top of a round trip
		// time.
		static constexpr int64_t BweDecreaseIntervalUs{ 300 * 1000 };
		// How long the target is allowed to just follow its bounds after the first
		// loss report.
		static constexpr int64_t StartPhaseUs{ 2000 * 1000 };
		// Packets a loss report must cover before its fraction of lost packets says
		// anything.
		static constexpr int64_t LimitNumPackets{ 20 };
		// Lowest bitrate this controller may ever produce.
		static constexpr int64_t CongestionControllerMinBitrate{ 5000 };
		// Upper bound used while the application sets no maximum.
		static constexpr int64_t DefaultMaxBitrate{ 1000000000 };
		// Age over which the latest loss report is not acted upon anymore. Reports
		// are expected within [0.5, 1.5] s intervals, so this is 1.2 times the
		// longest interval that still counts as uniform.
		static constexpr int64_t MaxLossReportAgeUs{ 6000 * 1000 };

		/* Instance methods. */

		TargetRateController::TargetRateController()
		  : TargetRateController(TargetRateControllerOptions{})
		{
			MS_TRACE();
		}

		TargetRateController::TargetRateController(TargetRateControllerOptions options)
		  : options(options),
		    minBitrateConfigured(CongestionControllerMinBitrate),
		    maxBitrateConfigured(DefaultMaxBitrate)
		{
			MS_TRACE();

			this->lossBasedController.SetBitrateLimits(
			  this->minBitrateConfigured, this->maxBitrateConfigured);
		}

		int64_t TargetRateController::GetTargetBitrate() const
		{
			MS_TRACE();

			int64_t target = this->currentTarget;

			// Unless it has been told to drive the target, the bound asked for by the
			// receiver caps only the value handed out, which is why it's applied here
			// and not in GetUpperLimit().
			if (!this->options.disableReceiverLimitCapsOnly)
			{
				target = std::min(target, this->receiverLimit);
			}

			return std::max(this->minBitrateConfigured, target);
		}

		void TargetRateController::OnRouteChange()
		{
			MS_TRACE();

			this->lostPacketsSinceLastLossUpdate     = 0;
			this->expectedPacketsSinceLastLossUpdate = 0;
			this->currentTarget                      = 0;
			this->minBitrateConfigured               = CongestionControllerMinBitrate;
			this->maxBitrateConfigured               = DefaultMaxBitrate;
			this->hasDecreasedSinceLastFractionLoss  = false;
			this->lastFractionLost                   = 0;
			this->lastRttUs                          = 0;
			this->receiverLimit                      = Types::BitrateInfinite;
			this->delayBasedLimit                    = Types::BitrateInfinite;
			this->firstLossReportAtUs.reset();
			this->lastLossReportAtUs.reset();
			this->lastDecreaseAtUs.reset();

			// A loss controller that is allowed to take over during the start phase
			// would otherwise carry what it learnt about the previous path into the
			// start phase that is about to begin again.
			if (this->lossBasedController.IsUsedInStartPhase())
			{
				this->lossBasedController.Reset();
			}
		}

		void TargetRateController::SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate)
		{
			MS_TRACE();

			this->minBitrateConfigured = std::max(minBitrate, CongestionControllerMinBitrate);

			// No maximum at all is not a maximum of every bitrate there is, it means
			// that the one this controller picks for itself applies.
			if (maxBitrate > 0 && maxBitrate != Types::BitrateInfinite)
			{
				this->maxBitrateConfigured = std::max(this->minBitrateConfigured, maxBitrate);
			}
			else
			{
				this->maxBitrateConfigured = DefaultMaxBitrate;
			}

			this->lossBasedController.SetBitrateLimits(
			  this->minBitrateConfigured, this->maxBitrateConfigured);
		}

		void TargetRateController::SetAcknowledgedBitrate(int64_t acknowledgedBitrate)
		{
			MS_TRACE();

			// Not knowing what the link delivers is not the same as it delivering
			// nothing, so the last figure that was known is kept instead.
			if (acknowledgedBitrate == Types::BitrateInfinite)
			{
				return;
			}

			// NOTE: Nothing else in here looks at it, it's only of use to the loss
			// controller.
			this->lossBasedController.SetAcknowledgedBitrate(acknowledgedBitrate);
		}

		void TargetRateController::UpdateLossBasedController(
		  const std::vector<Types::PacketResult>& packetResults, bool inAlr, int64_t nowUs)
		{
			MS_TRACE();

			this->lossBasedController.UpdateBitrateEstimate(packetResults, this->delayBasedLimit, inAlr);

			Update(nowUs);
		}

		void TargetRateController::SetSendBitrate(int64_t bitrate)
		{
			MS_TRACE();

			// Drop the bound of the delay based path so that it cannot cap the given
			// value.
			this->delayBasedLimit = Types::BitrateInfinite;

			// NOTE: A bitrate outside the configured bounds is brought into them
			// rather than rejected, since this value comes from the application.
			SetTargetBitrate(bitrate);

			// Clear the history so that the new value is grown from directly instead
			// of from whatever the target was before.
			this->minBitrateHistory.clear();
		}

		void TargetRateController::SetDelayBasedEstimate(int64_t bitrate)
		{
			MS_TRACE();

			this->delayBasedLimit = bitrate == 0 ? Types::BitrateInfinite : bitrate;

			ApplyTargetLimits();
		}

		void TargetRateController::SetReceiverEstimate(int64_t bitrate)
		{
			MS_TRACE();

			this->receiverLimit = bitrate == 0 ? Types::BitrateInfinite : bitrate;

			ApplyTargetLimits();
		}

		void TargetRateController::UpdatePacketsLost(int64_t lostPackets, int64_t totalPackets, int64_t nowUs)
		{
			MS_TRACE();

			if (!this->firstLossReportAtUs.has_value())
			{
				this->firstLossReportAtUs = nowUs;
			}

			if (totalPackets <= 0)
			{
				return;
			}

			const int64_t expectedPackets = this->expectedPacketsSinceLastLossUpdate + totalPackets;

			// Accumulate reports until they cover enough packets.
			if (expectedPackets < LimitNumPackets)
			{
				this->expectedPacketsSinceLastLossUpdate = expectedPackets;
				this->lostPacketsSinceLastLossUpdate += lostPackets;

				return;
			}

			this->hasDecreasedSinceLastFractionLoss = false;

			// NOTE: The reported fraction is in 1/256 units, hence the shift.
			const int64_t lostQ8 =
			  std::max<int64_t>(this->lostPacketsSinceLastLossUpdate + lostPackets, 0) << 8;

			this->lastFractionLost = static_cast<uint8_t>(std::min<int64_t>(lostQ8 / expectedPackets, 255));

			this->lostPacketsSinceLastLossUpdate     = 0;
			this->expectedPacketsSinceLastLossUpdate = 0;
			this->lastLossReportAtUs                 = nowUs;

			Update(nowUs);
		}

		void TargetRateController::UpdateRtt(int64_t rttUs)
		{
			MS_TRACE();

			// A round trip time cannot always be computed, since it needs a sender
			// report to refer to.
			if (rttUs > 0)
			{
				this->lastRttUs = rttUs;
			}
		}

		void TargetRateController::UpdatePropagationRtt(int64_t propagationRttUs, int64_t nowUs)
		{
			MS_TRACE();

			this->rttBackoff.lastPropagationRttAtUs = nowUs;
			this->rttBackoff.lastPropagationRttUs   = propagationRttUs;
		}

		void TargetRateController::OnPacketSent(int64_t sentAtUs)
		{
			MS_TRACE();

			this->rttBackoff.lastPacketSentAtUs = sentAtUs;
		}

		void TargetRateController::Update(int64_t nowUs)
		{
			MS_TRACE();

			// A round trip time this long means that the queues of the network are
			// full, which neither loss nor delay may have noticed yet.
			if (IsRttAboveLimit())
			{
				const bool mayDecrease =
				  !this->lastDecreaseAtUs.has_value() ||
				  nowUs - this->lastDecreaseAtUs.value() >= this->options.rttBackoffDropIntervalUs;

				if (mayDecrease && this->currentTarget > this->options.rttBackoffBitrateFloor)
				{
					this->lastDecreaseAtUs = nowUs;

					const auto bitrate = std::max<int64_t>(
					  Utils::ApplyBitrateFactor(this->currentTarget, this->options.rttBackoffDropFraction),
					  this->options.rttBackoffBitrateFloor);

					SetTargetBitrate(bitrate);
				}
				else
				{
					ApplyTargetLimits();
				}

				return;
			}

			// The bounds are trusted during the first seconds as long as no loss has
			// been reported, so that the initial probing can raise the target.
			if (this->lastFractionLost == 0 && IsInStartPhase(nowUs) && !this->lossBasedController.IsReadyToUseInStartPhase())
			{
				int64_t bitrate = this->currentTarget;

				if (this->receiverLimit != Types::BitrateInfinite)
				{
					bitrate = std::max(this->receiverLimit, bitrate);
				}

				if (this->delayBasedLimit != Types::BitrateInfinite)
				{
					bitrate = std::max(this->delayBasedLimit, bitrate);
				}

				if (bitrate != this->currentTarget)
				{
					// Take the target being left behind as the value to grow from, since
					// the new one was not reached by increasing.
					this->minBitrateHistory.clear();
					this->minBitrateHistory.emplace_back(nowUs, this->currentTarget);

					SetTargetBitrate(bitrate);

					return;
				}
			}

			UpdateMinBitrateHistory(nowUs);

			// Once the loss controller can tell congestion from the loss the link has
			// by itself, its estimate replaces the coarse rules below.
			if (this->lossBasedController.IsReady())
			{
				const auto result = this->lossBasedController.GetResult();

				this->lossBasedState = result.state;

				SetTargetBitrate(result.bitrate);

				return;
			}

			// No loss report has covered enough packets yet.
			if (!this->lastLossReportAtUs.has_value())
			{
				ApplyTargetLimits();

				return;
			}

			if (nowUs - this->lastLossReportAtUs.value() < MaxLossReportAgeUs)
			{
				const double loss = this->lastFractionLost / 256.0;

				if (this->currentTarget < this->options.bitrateThreshold || loss <= this->options.lowLossThreshold)
				{
					// Increase by 8% of the lowest target of the last second. Growing
					// from the lowest value instead of from the current one lets a sender
					// that was throttled ramp up a second faster.
					// Add a bit on top of the 8%, which is what keeps the target from
					// getting stuck at low bitrates and is negligible at high ones.
					const auto bitrate = Utils::AddBitrates(
					  Utils::ApplyBitrateFactor(this->minBitrateHistory.front().second, 1.08), 1000);

					SetTargetBitrate(bitrate);

					return;
				}

				// Loss between both thresholds means doing nothing at all.
				if (this->currentTarget > this->options.bitrateThreshold && loss > this->options.highLossThreshold)
				{
					const bool mayDecrease =
					  !this->lastDecreaseAtUs.has_value() ||
					  nowUs - this->lastDecreaseAtUs.value() >= BweDecreaseIntervalUs + this->lastRttUs;

					if (!this->hasDecreasedSinceLastFractionLoss && mayDecrease)
					{
						this->lastDecreaseAtUs = nowUs;

						// The new bitrate is the current one times (1 - 0.5 * lossRate),
						// where the reported fraction is 256 times that loss rate.
						const auto bitrate = static_cast<int64_t>(
						  (this->currentTarget * static_cast<double>(512 - this->lastFractionLost)) / 512.0);

						this->hasDecreasedSinceLastFractionLoss = true;

						SetTargetBitrate(bitrate);

						return;
					}
				}
			}

			ApplyTargetLimits();
		}

		bool TargetRateController::IsInStartPhase(int64_t nowUs) const
		{
			MS_TRACE();

			return !this->firstLossReportAtUs.has_value() ||
			       nowUs - this->firstLossReportAtUs.value() < StartPhaseUs;
		}

		void TargetRateController::UpdateMinBitrateHistory(int64_t nowUs)
		{
			MS_TRACE();

			// Drop the points that fell out of the window. The window is closed a
			// millisecond early, so that a target which has only just aged out of it
			// doesn't hold the increase back.
			while (!this->minBitrateHistory.empty() &&
			       nowUs - this->minBitrateHistory.front().first + 1000 > BweIncreaseIntervalUs)
			{
				this->minBitrateHistory.pop_front();
			}

			// Sliding window minimum: drop the points not lower than the current
			// target before pushing it, since they can never be the minimum again.
			while (!this->minBitrateHistory.empty() &&
			       this->currentTarget <= this->minBitrateHistory.back().second)
			{
				this->minBitrateHistory.pop_back();
			}

			this->minBitrateHistory.emplace_back(nowUs, this->currentTarget);
		}

		int64_t TargetRateController::GetUpperLimit() const
		{
			MS_TRACE();

			int64_t upperLimit = this->delayBasedLimit;

			if (this->options.disableReceiverLimitCapsOnly)
			{
				upperLimit = std::min(upperLimit, this->receiverLimit);
			}

			return std::min(upperLimit, this->maxBitrateConfigured);
		}

		void TargetRateController::SetTargetBitrate(int64_t bitrate)
		{
			MS_TRACE();

			this->currentTarget = std::max(this->minBitrateConfigured, std::min(bitrate, GetUpperLimit()));
		}

		void TargetRateController::ApplyTargetLimits()
		{
			MS_TRACE();

			SetTargetBitrate(this->currentTarget);
		}
	} // namespace BWE
} // namespace RTC
