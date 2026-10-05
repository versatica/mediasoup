#define MS_CLASS "RTC::BWE::SenderTransportCongestionController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/SenderTransportCongestionController.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BitrateUtils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How often everything that depends on time rather than on an arrival is run.
		// The shortest deadline it has to meet is the interval the loss based
		// controller waits before lowering the target again, so it is a good deal
		// finer than that.
		static constexpr int64_t ProcessIntervalMs{ 100 };
		// How far below the throughput a probe result may drag the estimate. A probe
		// far below what is already acknowledged is unlikely to be measuring the
		// link, but dropping to slightly below the throughput is what drains the
		// queues when there really is an overuse.
		static constexpr double ProbeDropThroughputFraction{ 0.85 };

		/* Static methods. */

		/**
		 * What is holding the target back, which is what tells the probe controller
		 * whether a probe would make any sense right now.
		 */
		static ProbeController::BandwidthLimitedCause getBandwidthLimitedCause(
		  LossBasedController::State lossBasedState,
		  bool isRttAboveLimit,
		  Types::BandwidthUsage bandwidthUsage)
		{
			if (bandwidthUsage == Types::BandwidthUsage::OVERUSING || bandwidthUsage == Types::BandwidthUsage::UNDERUSING)
			{
				return ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED_DELAY_INCREASED;
			}

			if (isRttAboveLimit)
			{
				return ProbeController::BandwidthLimitedCause::RTT_BASED_BACK_OFF_HIGH_RTT;
			}

			switch (lossBasedState)
			{
				case LossBasedController::State::DECREASING:
				{
					// No probe is sent in this state.
					return ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE;
				}

				case LossBasedController::State::INCREASING:
				{
					// Probes may be sent in this state.
					return ProbeController::BandwidthLimitedCause::LOSS_LIMITED_BWE_INCREASING;
				}

				case LossBasedController::State::DELAY_BASED_ESTIMATE:
				{
					return ProbeController::BandwidthLimitedCause::DELAY_BASED_LIMITED;
				}

					NO_DEFAULT();
			}
		}

		/* Instance methods. */

		SenderTransportCongestionController::SenderTransportCongestionController(
		  Listener* listener, SharedInterface* shared, SenderTransportCongestionControllerOptions options)
		  : listener(listener),
		    shared(shared),
		    processTimer(shared->CreateTimer(this, "sender-transport-congestion-controller-process")),
		    feedbackAdapter(std::addressof(this->sendPacketHistory)),
		    alrDetector(shared),
		    probingScheduler(this, shared),
		    startBitrate(options.startBitrate)
		{
			MS_TRACE();

			this->minBitrate = options.minBitrate;
			this->maxBitrate = options.maxBitrate;

			ResetConstraints(this->shared->GetTimeUs(), /*applyStartBitrate*/ true);

			this->processTimer->Start(ProcessIntervalMs, ProcessIntervalMs);
		}

		void SenderTransportCongestionController::SetNetworkAvailable(bool networkAvailable)
		{
			MS_TRACE();

			const int64_t nowUs = this->shared->GetTimeUs();

			CreateProbeClusters(this->probeController.OnNetworkAvailability(networkAvailable, nowUs));
		}

		int64_t SenderTransportCongestionController::OnRtpPacketToBeSent(
		  RTC::RTP::Packet* packet, const RtpPacketToBeSentOptions& options)
		{
			MS_TRACE();

			return AddRtpPacket(packet, options, /*probeCluster*/ std::nullopt);
		}

		void SenderTransportCongestionController::OnRtpPacketSent(int64_t sequenceNumber, int64_t sentAtUs)
		{
			MS_TRACE();

			const auto sentPacket = this->sendPacketHistory.ProcessSentPacket(sequenceNumber, sentAtUs);

			// Nothing is known about a packet the history does not hold, so there is
			// nothing to tell anybody about it either.
			if (!sentPacket.has_value())
			{
				return;
			}

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const auto& sentPacketValue = sentPacket.value();

			this->alrDetector.OnBytesSent(static_cast<int64_t>(sentPacketValue.size), sentAtUs);
			this->acknowledgedBitrateEstimator.SetAlr(this->alrDetector.GetAlrStartTimeUs().has_value());

			if (!this->firstPacketSent)
			{
				this->firstPacketSent = true;

				// Until a feedback arrives there is no round trip to measure, so the
				// instant the first packet left is taken as the start of one. Without
				// this the backoff by round trip time would have nothing to work with
				// during the first seconds.
				this->targetRateController.UpdatePropagationRtt(0, sentAtUs);
			}

			this->targetRateController.OnPacketSent(sentAtUs);
		}

		void SenderTransportCongestionController::ReceiveTransportWideCcFeedback(
		  const RTC::RTCP::FeedbackRtpTransportPacket* feedback, int64_t receivedAtUs)
		{
			MS_TRACE();

			const auto processedFeedback =
			  this->feedbackAdapter.ProcessTransportWideCcFeedback(feedback, receivedAtUs);

			if (!processedFeedback.has_value())
			{
				return;
			}

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const auto& transportPacketsFeedback = processedFeedback.value();

			if (transportPacketsFeedback.packetFeedbacks.empty())
			{
				return;
			}

			// Something reports back, so the estimation is ours to do and not the
			// remote endpoint's.
			this->firstTransportFeedbackReceived = true;

			const int64_t feedbackTimeUs    = transportPacketsFeedback.feedbackTimeUs;
			const auto receivedWithSendInfo = transportPacketsFeedback.ReceivedWithSendInfo();

			// The round trip of the whole feedback is the time since the oldest packet
			// it reports on left, minus how long the rest of them waited at the remote
			// endpoint before it decided to report. Taking the smallest of those is
			// what leaves the propagation alone, without the wait.
			int64_t minPropagationRttUs{ Types::TimeUsInfinite };
			int64_t maxReceiveTimeUs{ 0 };

			for (const auto& packetResult : receivedWithSendInfo)
			{
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				maxReceiveTimeUs = std::max(maxReceiveTimeUs, packetResult.receiveTimeUs.value());
			}

			for (const auto& packetResult : receivedWithSendInfo)
			{
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				const int64_t receiveTimeUs    = packetResult.receiveTimeUs.value();
				const int64_t feedbackRttUs    = feedbackTimeUs - packetResult.sentPacket.sendTimeUs;
				const int64_t minPendingTimeUs = maxReceiveTimeUs - receiveTimeUs;
				const int64_t propagationRttUs = feedbackRttUs - minPendingTimeUs;

				minPropagationRttUs = std::min(minPropagationRttUs, propagationRttUs);
			}

			if (!receivedWithSendInfo.empty())
			{
				this->targetRateController.UpdatePropagationRtt(minPropagationRttUs, feedbackTimeUs);
			}

			const auto alrStartTimeUs = this->alrDetector.GetAlrStartTimeUs();

			// Leaving the state has to be told to both of them, since each one holds
			// its own idea of when it ended.
			if (this->previouslyInAlr && !alrStartTimeUs.has_value())
			{
				this->acknowledgedBitrateEstimator.SetAlrEndedTimeUs(feedbackTimeUs);
				this->probeController.SetAlrEndedTimeUs(feedbackTimeUs);
			}

			this->previouslyInAlr = alrStartTimeUs.has_value();

			this->acknowledgedBitrateEstimator.IncomingPacketFeedbackVector(
			  transportPacketsFeedback.SortedByReceiveTime());

			const auto acknowledgedBitrate = this->acknowledgedBitrateEstimator.GetBitrate();

			this->targetRateController.SetAcknowledgedBitrate(
			  acknowledgedBitrate.value_or(Types::BitrateInfinite));

			for (const auto& packetResult : transportPacketsFeedback.SortedByReceiveTime())
			{
				if (packetResult.sentPacket.probeCluster.has_value())
				{
					this->probeBitrateEstimator.HandleProbeAndEstimateBitrate(packetResult);
				}
			}

			auto probeBitrate = this->probeBitrateEstimator.FetchAndResetLastEstimatedBitrate();

			// A probe that measured far below what is already acknowledged is not
			// measuring the link, so the drop it would cause is bounded.
			if (probeBitrate.has_value() && acknowledgedBitrate.has_value())
			{
				const int64_t limit = std::min(
				  this->delayBasedBwe.GetLastEstimate(),
				  BitrateUtils::ApplyBitrateFactor(
				    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				    acknowledgedBitrate.value(),
				    ProbeDropThroughputFraction));

				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				probeBitrate = std::max(probeBitrate.value(), limit);
			}

			const auto result = this->delayBasedBwe.IncomingPacketFeedbackVector(
			  transportPacketsFeedback,
			  acknowledgedBitrate,
			  probeBitrate,
			  /*networkEstimate*/ std::nullopt,
			  alrStartTimeUs.has_value());

			if (result.updated)
			{
				// A probe result replaces the estimate instead of nudging it, and that
				// has to happen before the delay based estimate is handed over, since
				// setting it clears what the delay based path had.
				if (result.probe)
				{
					this->targetRateController.SetSendBitrate(result.targetBitrate);
				}

				this->targetRateController.SetDelayBasedEstimate(result.targetBitrate);
			}

			this->targetRateController.UpdateLossBasedController(
			  transportPacketsFeedback.packetFeedbacks, alrStartTimeUs.has_value(), feedbackTimeUs);

			if (result.updated)
			{
				MayNotifyTargetBitrate(feedbackTimeUs);
			}

			// Coming out of an overuse is the one moment where a probe is worth it
			// without waiting for anything else.
			if (result.recoveredFromOveruse)
			{
				this->probeController.SetAlrStartTimeUs(alrStartTimeUs);

				CreateProbeClusters(this->probeController.RequestProbe(feedbackTimeUs));
			}
		}

		void SenderTransportCongestionController::ReceiveRtcpReceiverReport(
		  RTC::RTCP::ReceiverReportPacket* packet, int64_t receivedAtUs)
		{
			MS_TRACE();

			const auto loss = this->packetLossTracker.ReceiveReceiverReport(packet);

			if (!loss.has_value())
			{
				return;
			}

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const auto& lossValue = loss.value();

			this->targetRateController.UpdatePacketsLost(
			  lossValue.lostPackets, lossValue.expectedPackets, this->shared->GetTimeUs());
		}

		void SenderTransportCongestionController::ReceiveEstimatedBitrate(int64_t bitrate)
		{
			MS_TRACE();

			this->targetRateController.SetReceiverEstimate(bitrate);
		}

		void SenderTransportCongestionController::OnRttUpdate(int64_t rttUs)
		{
			MS_TRACE();

			// NOTE: The round trip is taken in whole milliseconds, which is the
			// resolution the thresholds it is compared against are expressed in.
			const int64_t roundedRttUs = Utils::Time::TimeUsToMs(rttUs) * 1000;

			if (roundedRttUs == 0)
			{
				return;
			}

			this->delayBasedBwe.OnRttUpdate(roundedRttUs);
			this->targetRateController.UpdateRtt(roundedRttUs);
		}

		void SenderTransportCongestionController::RemoveStream(uint32_t ssrc)
		{
			MS_TRACE();

			this->packetLossTracker.RemoveStream(ssrc);
		}

		void SenderTransportCongestionController::SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate)
		{
			MS_TRACE();

			const int64_t nowUs = this->shared->GetTimeUs();

			this->minBitrate = minBitrate;
			this->maxBitrate = maxBitrate;

			ResetConstraints(nowUs, /*applyStartBitrate*/ false);

			MayNotifyTargetBitrate(nowUs);
		}

		void SenderTransportCongestionController::SetDesiredBitrate(int64_t desiredBitrate)
		{
			MS_TRACE();

			CreateProbeClusters(this->probeController.OnMaxTotalAllocatedBitrate(
			  desiredBitrate, this->shared->GetTimeUs()));
		}

		void SenderTransportCongestionController::SetPacketOverhead(size_t packetOverhead)
		{
			MS_TRACE();

			this->packetOverhead = packetOverhead;

			this->probingScheduler.SetPacketOverhead(packetOverhead);
		}

		int64_t SenderTransportCongestionController::GetAvailableBitrate() const
		{
			MS_TRACE();

			return this->targetRateController.GetTargetBitrate();
		}

		bool SenderTransportCongestionController::OnProbingSchedulerSendRtpPacket(
		  ProbingScheduler* /*probingScheduler*/,
		  RTC::RTP::Packet* packet,
		  const Types::ProbeCluster& probeCluster)
		{
			MS_TRACE();

			// The generator leaves room for both extensions but writes neither, and
			// since it reuses a single packet what is left unwritten is whatever the
			// previous one carried.
			AddRtpPacket(packet, RtpPacketToBeSentOptions{}, probeCluster);

			return this->listener->OnSenderTransportCongestionControllerSendRtpPacket(this, packet);
		}

		void SenderTransportCongestionController::OnTimer(TimerHandleInterface* timer)
		{
			MS_TRACE();

			if (timer == this->processTimer.get())
			{
				Process();
			}
		}

		int64_t SenderTransportCongestionController::AddRtpPacket(
		  RTC::RTP::Packet* packet,
		  const RtpPacketToBeSentOptions& options,
		  std::optional<Types::ProbeCluster> probeCluster)
		{
			MS_TRACE();

			const int64_t nowUs = this->shared->GetTimeUs();

			const int64_t sequenceNumber = this->sendPacketHistory.AddPacket(
			  SendPacketHistory::AddPacketOptions{
			    .ssrc = packet->GetSsrc(),
			    .seq  = packet->GetSequenceNumber(),
			    // What the link carries is the packet plus everything below RTP, so
			    // that is what the rate control has to reason about.
			    .size             = packet->GetLength() + this->packetOverhead,
			    .isAudio          = options.isAudio,
			    .isRetransmission = options.isRetransmission,
			    .originalSsrc     = options.originalSsrc,
			    .probeCluster     = probeCluster,
			    .createdAtUs      = nowUs });

			packet->UpdateTransportWideCc01(static_cast<uint16_t>(sequenceNumber));
			packet->UpdateAbsSendTime(Utils::Time::TimeUsToAbsSendTime(nowUs));

			return sequenceNumber;
		}

		void SenderTransportCongestionController::Process()
		{
			MS_TRACE();

			const int64_t nowUs = this->shared->GetTimeUs();

			this->targetRateController.Update(nowUs);

			// Probing while the remote endpoint is the one estimating would be probing
			// something nobody is measuring, so it waits for the first feedback.
			this->probeController.SetAlrStartTimeUs(
			  this->firstTransportFeedbackReceived ? this->alrDetector.GetAlrStartTimeUs() : std::nullopt);

			CreateProbeClusters(this->probeController.Process(nowUs));

			MayNotifyTargetBitrate(nowUs);
		}

		void SenderTransportCongestionController::CreateProbeClusters(
		  const std::vector<Types::ProbeClusterConfig>& clusterConfigs)
		{
			MS_TRACE();

			if (clusterConfigs.empty())
			{
				return;
			}

			this->probingScheduler.CreateProbeClusters(clusterConfigs);
		}

		void SenderTransportCongestionController::MayNotifyTargetBitrate(int64_t nowUs)
		{
			MS_TRACE();

			const int64_t targetBitrate = this->targetRateController.GetTargetBitrate();
			const uint8_t fractionLost  = this->targetRateController.GetFractionLost();
			const int64_t rttUs         = this->targetRateController.GetRttUs();
			const LossBasedController::State lossBasedState =
			  this->targetRateController.GetLossBasedState();
			// Not filling the link is not the same as not being able to fill it, and
			// what is announced is whether the link is what holds the sender back.
			const bool isBandwidthLimited = !this->alrDetector.GetAlrStartTimeUs().has_value();

			// Everything the listener reasons about is compared, since a reallocation
			// is far more expensive than this comparison.
			if (
			  this->lastTargetBitrate.has_value() && targetBitrate == this->lastTargetBitrate.value() &&
			  fractionLost == this->lastFractionLost && rttUs == this->lastRttUs &&
			  lossBasedState == this->lastLossBasedState &&
			  isBandwidthLimited == this->lastIsBandwidthLimited)
			{
				return;
			}

			this->lastTargetBitrate      = targetBitrate;
			this->lastFractionLost       = fractionLost;
			this->lastRttUs              = rttUs;
			this->lastLossBasedState     = lossBasedState;
			this->lastIsBandwidthLimited = isBandwidthLimited;

			this->alrDetector.SetEstimatedBitrate(targetBitrate);

			CreateProbeClusters(this->probeController.SetEstimatedBitrate(
			  targetBitrate,
			  getBandwidthLimitedCause(
			    lossBasedState,
			    this->targetRateController.IsRttAboveLimit(),
			    this->delayBasedBwe.GetLastState()),
			  nowUs));

			this->listener->OnSenderTransportCongestionControllerTargetBitrate(this, targetBitrate);
		}

		void SenderTransportCongestionController::ResetConstraints(int64_t nowUs, bool applyStartBitrate)
		{
			MS_TRACE();

			// Nothing below may be driven to a bitrate it treats as none at all, since
			// with nothing going out there would be nothing left to measure.
			this->minBitrate = std::max(this->minBitrate, RTC::Consts::BweMinBitrate);

			if (this->maxBitrate < this->minBitrate)
			{
				MS_WARN_TAG(
				  bwe,
				  "max bitrate is lower than min bitrate [maxBitrate:%" PRIi64 ", minBitrate:%" PRIi64 "]",
				  this->maxBitrate,
				  this->minBitrate);

				this->maxBitrate = this->minBitrate;
			}

			// NOTE: Raised here and not where it is kept, so that the member stays
			// what was configured.
			const int64_t startBitrate =
			  applyStartBitrate ? std::max(this->startBitrate, this->minBitrate) : 0;

			this->targetRateController.SetBitrateLimits(this->minBitrate, this->maxBitrate);

			this->delayBasedBwe.SetMinBitrate(this->minBitrate);

			if (applyStartBitrate)
			{
				this->targetRateController.SetSendBitrate(startBitrate);
				this->delayBasedBwe.SetStartBitrate(startBitrate);
			}

			CreateProbeClusters(
			  this->probeController.SetBitrates(this->minBitrate, startBitrate, this->maxBitrate, nowUs));
		}
	} // namespace BWE
} // namespace RTC
