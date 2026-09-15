#define MS_CLASS "RTC::BWE::TargetRateController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/TargetRateController.hpp"
#include "Logger.hpp"
#include <cmath>

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
		}

		TargetRateController::TargetRateController(TargetRateControllerOptions options)
		  : options(options),
		    minBitrateConfigured(CongestionControllerMinBitrate),
		    maxBitrateConfigured(DefaultMaxBitrate)
		{
			MS_TRACE();

			// TODO: Invoke this->lossBasedController.SetBitrateLimits(
			//   this->minBitrateConfigured, this->maxBitrateConfigured).
		}

		int64_t TargetRateController::GetTargetBitrate() const
		{
			MS_TRACE();

			// The bound asked for by the receiver caps what this controller produces
			// but doesn't take part in how the target moves, so it's applied here and
			// not in GetUpperLimit().
			const int64_t target = std::min(this->currentTarget, this->receiverLimit);

			return std::max(this->minBitrateConfigured, target);
		}

		void TargetRateController::SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate)
		{
			MS_TRACE();

			this->minBitrateConfigured = std::max(minBitrate, CongestionControllerMinBitrate);

			if (maxBitrate > 0)
			{
				this->maxBitrateConfigured = std::max(this->minBitrateConfigured, maxBitrate);
			}
			else
			{
				this->maxBitrateConfigured = DefaultMaxBitrate;
			}

			// TODO: Invoke this->lossBasedController.SetBitrateLimits(
			//   this->minBitrateConfigured, this->maxBitrateConfigured).
		}

		// TODO: A SetAcknowledgedBitrate(std::optional<int64_t> acknowledgedBitrate)
		// method is missing, whose only job is invoking
		// this->lossBasedController.SetAcknowledgedBitrate(). It was left out because
		// the acknowledged bitrate is not used by anything else in this class.

		// TODO: An UpdateLossBasedController(packetResults, inAlr) method is missing,
		// which is how the per packet feedback reaches the loss controller. It must
		// invoke this->lossBasedController.UpdateBandwidthEstimate(packetResults,
		// this->delayBasedLimit, inAlr) and then Update() with the instant of the
		// feedback.

		// TODO: A GetLossBasedState() getter is missing, which just exposes the state
		// of the loss controller.

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
					  static_cast<int64_t>(this->currentTarget * this->options.rttBackoffDropFraction),
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
			// TODO: This condition gains a third clause,
			// && !this->lossBasedController.IsReadyToUseInStartPhase().
			if (this->lastFractionLost == 0 && IsInStartPhase(nowUs))
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

			// TODO: The switch to the loss controller goes here: once it's ready its
			// estimate replaces the rules below, so this must take its result, set the
			// target with it and return.

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
					auto bitrate =
					  static_cast<int64_t>(std::lround(this->minBitrateHistory.front().second * 1.08));

					// Add a bit on top, which is what keeps the target from getting stuck
					// at low bitrates and is negligible at high ones.
					bitrate += 1000;

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

			// Drop the points that fell out of the window.
			while (!this->minBitrateHistory.empty() &&
			       nowUs - this->minBitrateHistory.front().first > BweIncreaseIntervalUs)
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

			return std::min(this->delayBasedLimit, this->maxBitrateConfigured);
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
