#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV_LEVEL 3
#define USE_TREND_CALCULATOR

#include "RTC/TransportCongestionControlClient.hpp"
#include "Logger.hpp"
#include <libwebrtc/api/transport/network_types.h> // webrtc::TargetRateConstraints
#include <cmath>                                   // std::llround()
#include <limits>                                  // std::numeric_limits

namespace RTC
{
	/* Static. */

	// NOTE: TransportCongestionControlMinOutgoingBitrate is defined in
	// TransportCongestionControlClient.hpp and exposed publicly.
	// NOTE: These are double rather than float because they are applied to bitrates,
	// and float only holds exact integers up to 2^24 (16.7 Mbps).
	static constexpr double MaxBitrateMarginFactor{ 0.1 };
	static constexpr double MaxBitrateIncrementFactor{ 1.35 };
	static constexpr double MaxPaddingBitrateFactor{ 0.85 };
	static constexpr int64_t AvailableBitrateEventIntervalMs{ 1000 };
	static constexpr size_t PacketLossHistogramLength{ 24 };

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener,
	  SharedInterface* shared,
	  RTC::BweType bweType,
	  int64_t initialAvailableBitrate,
	  int64_t maxOutgoingBitrate,
	  int64_t minOutgoingBitrate)
	  : listener(listener),
	    shared(shared),
	    bweType(bweType),
	    initialAvailableBitrate(
	      std::max<int64_t>(initialAvailableBitrate, RTC::TransportCongestionControlMinOutgoingBitrate)),
	    maxOutgoingBitrate(maxOutgoingBitrate),
	    minOutgoingBitrate(minOutgoingBitrate)
	{
		MS_TRACE();

		webrtc::GoogCcFactoryConfig config;

		// Provide RTCP feedback as well as Receiver Reports.
		config.feedback_only = true;

		this->controllerFactory = new webrtc::GoogCcNetworkControllerFactory(std::move(config));
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		delete this->controllerFactory;
		this->controllerFactory = nullptr;

		DestroyController();
	}

	void TransportCongestionControlClient::InitializeController()
	{
		MS_TRACE();

		MS_ASSERT(this->rtpTransportControllerSend == nullptr, "transport controller already initialized");

		webrtc::BitrateConstraints bitrateConfig;

		// NOTE: The dependency takes a signed 32 bits bitrate, so it is clamped here.
		bitrateConfig.start_bitrate_bps = static_cast<int>(
		  std::min<int64_t>(this->initialAvailableBitrate, std::numeric_limits<int>::max()));

		this->rtpTransportControllerSend =
		  new webrtc::RtpTransportControllerSend(this, nullptr, this->controllerFactory, bitrateConfig);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->probationGenerator = new RTC::RTP::ProbationGenerator();

		// This makes sure that periodic probing is used when the application is send
		// less bitrate than needed to measure the bandwidth estimation.  (f.e. when
		// videos are muted or using screensharing with still images)
		this->rtpTransportControllerSend->EnablePeriodicAlrProbing(true);

		this->processTimer =
		  this->shared->CreateTimer(this, "transport-congestion-control-client-process");

		this->processTimer->Start(
		  std::min(
		    // Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
		    this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
		    // Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
		    this->controllerFactory->GetProcessInterval().ms()));
	}

	void TransportCongestionControlClient::DestroyController()
	{
		MS_TRACE();

		delete this->rtpTransportControllerSend;
		this->rtpTransportControllerSend = nullptr;

		delete this->probationGenerator;
		this->probationGenerator = nullptr;

		delete this->processTimer;
		this->processTimer = nullptr;
	}

	void TransportCongestionControlClient::TransportConnected()
	{
		MS_TRACE();

		if (this->rtpTransportControllerSend == nullptr)
		{
			InitializeController();
		}

		this->rtpTransportControllerSend->OnNetworkAvailability(true);
	}

	void TransportCongestionControlClient::TransportDisconnected()
	{
		MS_TRACE();

#ifdef USE_TREND_CALCULATOR
		const auto nowMs = this->shared->GetTimeMs();
#endif

		this->bitrates.desiredBitrate          = 0;
		this->bitrates.effectiveDesiredBitrate = 0;

#ifdef USE_TREND_CALCULATOR
		this->desiredBitrateTrend.ForceUpdate(0, nowMs);
#endif

		this->rtpTransportControllerSend->OnNetworkAvailability(false);
	}

	void TransportCongestionControlClient::InsertPacket(webrtc::RtpPacketSendInfo& packetInfo)
	{
		MS_TRACE();

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		this->rtpTransportControllerSend->packet_sender()->InsertPacket(packetInfo.length);
		this->rtpTransportControllerSend->OnAddPacket(packetInfo);
	}

	webrtc::PacedPacketInfo TransportCongestionControlClient::GetPacingInfo()
	{
		MS_TRACE();

		if (this->rtpTransportControllerSend == nullptr)
		{
			return {};
		}

		return this->rtpTransportControllerSend->packet_sender()->GetPacingInfo();
	}

	void TransportCongestionControlClient::PacketSent(
	  const webrtc::RtpPacketSendInfo& packetInfo, int64_t nowUs)
	{
		MS_TRACE();

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		// Notify the transport feedback adapter about the sent packet.
		// NOTE: The send time is truncated to whole milliseconds because that's the
		// resolution the current estimator takes.
		const rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, nowUs / 1000);

		this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);
	}

	void TransportCongestionControlClient::ReceiveEstimatedBitrate(int64_t bitrate)
	{
		MS_TRACE();

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		// NOTE: The dependency takes an unsigned 32 bits bitrate, so it is clamped here.
		this->rtpTransportControllerSend->OnReceivedEstimatedBitrate(
		  static_cast<uint32_t>(std::clamp<int64_t>(bitrate, 0, std::numeric_limits<uint32_t>::max())));
	}

	void TransportCongestionControlClient::ReceiveRtcpReceiverReport(
	  RTC::RTCP::ReceiverReportPacket* packet, float rttMs, int64_t receivedAtUs)
	{
		MS_TRACE();

		webrtc::ReportBlockList reportBlockList;

		for (auto it = packet->Begin(); it != packet->End(); ++it)
		{
			auto& report = *it;

			reportBlockList.emplace_back(
			  packet->GetSsrc(),
			  report->GetSsrc(),
			  report->GetFractionLost(),
			  report->GetTotalLost(),
			  report->GetLastSeq(),
			  report->GetJitter(),
			  report->GetLastSenderReport(),
			  report->GetDelaySinceLastSenderReport());
		}

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		// NOTE: The dependency works in milliseconds, so the arrival time is
		// truncated here.
		this->rtpTransportControllerSend->OnReceivedRtcpReceiverReport(
		  reportBlockList, static_cast<int64_t>(rttMs), receivedAtUs / 1000);
	}

	void TransportCongestionControlClient::ReceiveRtcpTransportFeedback(
	  const RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		// Update packet loss history.
		const size_t expectedPackets = feedback->GetPacketStatusCount();
		size_t lostPackets           = 0;

		for (const auto& packetStatus : feedback->GetPacketStatuses())
		{
			if (!packetStatus.received)
			{
				lostPackets += 1;
			}
		}

		if (expectedPackets > 0)
		{
			this->UpdatePacketLoss(static_cast<double>(lostPackets) / expectedPackets);
		}

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		this->rtpTransportControllerSend->OnTransportFeedback(*feedback);
	}

	void TransportCongestionControlClient::UpdatePacketLoss(double packetLoss)
	{
		MS_TRACE();

		// Add the lost into the histogram.
		if (this->packetLossHistory.size() == PacketLossHistogramLength)
		{
			this->packetLossHistory.pop_front();
		}

		this->packetLossHistory.push_back(packetLoss);

		/*
		 * Scoring mechanism is a weighted average.
		 *
		 * The more recent the score is, the more weight it has.
		 * The oldest score has a weight of 1 and subsequent scores weight is
		 * increased by one sequentially.
		 *
		 * Ie:
		 * - scores: [1,2,3,4]
		 * - this->scores = ((1) + (2+2) + (3+3+3) + (4+4+4+4)) / 10 = 2.8 => 3
		 */

		size_t weight{ 0 };
		size_t samples{ 0 };
		double totalPacketLoss{ 0 };

		for (auto packetLossEntry : this->packetLossHistory)
		{
			weight++;
			samples += weight;
			totalPacketLoss += weight * packetLossEntry;
		}

		// clang-tidy "thinks" that this can lead to division by zero but we are
		// smarter.
		// NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
		this->packetLoss = totalPacketLoss / samples;
	}

	void TransportCongestionControlClient::SetMaxOutgoingBitrate(int64_t maxBitrate)
	{
		MS_TRACE();

		this->maxOutgoingBitrate = maxBitrate;

		ApplyBitrateUpdates();

		if (this->maxOutgoingBitrate > 0)
		{
			this->bitrates.availableBitrate =
			  std::min<int64_t>(this->maxOutgoingBitrate, this->bitrates.availableBitrate);
		}
	}

	void TransportCongestionControlClient::SetMinOutgoingBitrate(int64_t minBitrate)
	{
		MS_TRACE();

		this->minOutgoingBitrate = minBitrate;

		ApplyBitrateUpdates();

		this->bitrates.minBitrate =
		  std::max<int64_t>(this->minOutgoingBitrate, RTC::TransportCongestionControlMinOutgoingBitrate);
	}

	void TransportCongestionControlClient::SetDesiredBitrate(int64_t desiredBitrate, bool force)
	{
		MS_TRACE();

#ifdef USE_TREND_CALCULATOR
		const auto nowMs = this->shared->GetTimeMs();
#endif

		// Manage it via trending and increase it a bit to avoid immediate oscillations.
#ifdef USE_TREND_CALCULATOR
		if (!force)
		{
			this->desiredBitrateTrend.Update(desiredBitrate, nowMs);
		}
		else
		{
			this->desiredBitrateTrend.ForceUpdate(desiredBitrate, nowMs);
		}
#endif

		this->bitrates.desiredBitrate = desiredBitrate;

#ifdef USE_TREND_CALCULATOR
		this->bitrates.effectiveDesiredBitrate = this->desiredBitrateTrend.GetValue();
#else
		this->bitrates.effectiveDesiredBitrate = desiredBitrate;
#endif

		this->bitrates.minBitrate =
		  std::max<int64_t>(this->minOutgoingBitrate, RTC::TransportCongestionControlMinOutgoingBitrate);

		// NOTE: Setting 'startBitrate' to 'availableBitrate' has proven to generate
		// more stable values.
		this->bitrates.startBitrate = std::max<int64_t>(
		  RTC::TransportCongestionControlMinOutgoingBitrate, this->bitrates.availableBitrate);

		ApplyBitrateUpdates();
	}

	void TransportCongestionControlClient::ApplyBitrateUpdates()
	{
		MS_TRACE();

		auto currentMaxBitrate = this->bitrates.maxBitrate;
		int64_t newMaxBitrate  = 0;

#ifdef USE_TREND_CALCULATOR
		if (this->desiredBitrateTrend.GetValue() > 0)
#else
		if (this->bitrates.desiredBitrate > 0)
#endif
		{
			newMaxBitrate = std::max<int64_t>(
			  this->initialAvailableBitrate,
#ifdef USE_TREND_CALCULATOR
			  std::llround(this->desiredBitrateTrend.GetValue() * MaxBitrateIncrementFactor));
#else
			  std::llround(this->bitrates.desiredBitrate * MaxBitrateIncrementFactor));
#endif

			// If max bitrate requested didn't change by more than a small % keep the
			// previous settings to avoid constant small fluctuations requiring extra
			// probing and making the estimation less stable (requires constant
			// redistribution of bitrate accross consumers).
			const int64_t maxBitrateMargin = std::llround(newMaxBitrate * MaxBitrateMarginFactor);

			if (currentMaxBitrate > newMaxBitrate - maxBitrateMargin && currentMaxBitrate < newMaxBitrate + maxBitrateMargin)
			{
				newMaxBitrate = currentMaxBitrate;
			}
		}
		else
		{
			newMaxBitrate = this->initialAvailableBitrate;
		}

		if (this->maxOutgoingBitrate > 0)
		{
			newMaxBitrate = std::min<int64_t>(this->maxOutgoingBitrate, newMaxBitrate);
		}

		if (newMaxBitrate != currentMaxBitrate)
		{
			this->bitrates.maxPaddingBitrate = std::llround(newMaxBitrate * MaxPaddingBitrateFactor);
			this->bitrates.maxBitrate        = newMaxBitrate;
		}

		this->bitrates.minBitrate =
		  std::max<int64_t>(this->minOutgoingBitrate, RTC::TransportCongestionControlMinOutgoingBitrate);

		MS_DEBUG_DEV(
		  "[desiredBitrate:%" PRIi64 ", desiredBitrateTrend:%" PRIu32 ", startBitrate:%" PRIi64
		  ", minBitrate:%" PRIi64 ", maxBitrate:%" PRIi64 ", maxPaddingBitrate:%" PRIi64 "]",
		  this->bitrates.desiredBitrate,
		  this->desiredBitrateTrend.GetValue(),
		  this->bitrates.startBitrate,
		  this->bitrates.minBitrate,
		  this->bitrates.maxBitrate,
		  this->bitrates.maxPaddingBitrate);

		if (this->rtpTransportControllerSend == nullptr)
		{
			return;
		}

		this->rtpTransportControllerSend->SetAllocatedSendBitrateLimits(
		  this->bitrates.minBitrate, this->bitrates.maxPaddingBitrate, this->bitrates.maxBitrate);

		webrtc::TargetRateConstraints constraints;

		constraints.at_time       = webrtc::Timestamp::ms(this->shared->GetTimeMs());
		constraints.min_data_rate = webrtc::DataRate::bps(this->bitrates.minBitrate);
		constraints.max_data_rate = webrtc::DataRate::bps(this->bitrates.maxBitrate);
		constraints.starting_rate = webrtc::DataRate::bps(this->bitrates.startBitrate);

		this->rtpTransportControllerSend->SetClientBitratePreferences(constraints);
	}

	int64_t TransportCongestionControlClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->bitrates.availableBitrate;
	}

	double TransportCongestionControlClient::GetPacketLoss() const
	{
		MS_TRACE();

		return this->packetLoss;
	}

	void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAtMs = this->shared->GetTimeMs();
	}

	void TransportCongestionControlClient::MayEmitAvailableBitrateEvent(int64_t previousAvailableBitrate)
	{
		MS_TRACE();

		const int64_t nowMs = this->shared->GetTimeMs();
		bool notify{ false };

		// Ignore if first event.
		// NOTE: Otherwise it will make the Transport crash since this event also happens
		// during the constructor of this class.
		if (this->lastAvailableBitrateEventAtMs == 0)
		{
			this->lastAvailableBitrateEventAtMs = nowMs;

			return;
		}

		// Emit if this is the first valid event.
		if (!this->availableBitrateEventCalled)
		{
			this->availableBitrateEventCalled = true;

			notify = true;
		}
		// Emit event if AvailableBitrateEventIntervalMs elapsed.
		else if (nowMs - this->lastAvailableBitrateEventAtMs >= AvailableBitrateEventIntervalMs)
		{
			notify = true;
		}
		// Also emit the event fast if we detect a high BWE value decrease.
		else if (this->bitrates.availableBitrate < previousAvailableBitrate * 0.75)
		{
			MS_WARN_TAG(
			  bwe,
			  "high BWE value decrease detected, notifying the listener [now:%" PRIi64 ", before:%" PRIi64
			  "]",
			  this->bitrates.availableBitrate,
			  previousAvailableBitrate);

			notify = true;
		}
		// Also emit the event fast if we detect a high BWE value increase.
		else if (this->bitrates.availableBitrate > previousAvailableBitrate * 1.50)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "high BWE value increase detected, notifying the listener [now:%" PRIi64 ", before:%" PRIi64
			  "]",
			  this->bitrates.availableBitrate,
			  previousAvailableBitrate);

			notify = true;
		}

		if (notify)
		{
			MS_DEBUG_DEV(
			  "notifying the listener with new available bitrate:%" PRIi64,
			  this->bitrates.availableBitrate);

			this->lastAvailableBitrateEventAtMs = nowMs;

			this->listener->OnTransportCongestionControlClientBitrates(this, this->bitrates);
		}
	}

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		// NOTE: The same value as 'this->initialAvailableBitrate' is received
		// periodically regardless of the real available bitrate. Skip such value
		// except for the first time this event is called.
		if (this->availableBitrateEventCalled && targetTransferRate.target_rate.bps() == this->initialAvailableBitrate)
		{
			return;
		}

		auto previousAvailableBitrate = this->bitrates.availableBitrate;

		// Update availableBitrate.
		// NOTE: The dependency gives a signed 64 bits bitrate, so no clamping is
		// needed other than discarding a negative value.
		this->bitrates.availableBitrate = std::max<int64_t>(targetTransferRate.target_rate.bps(), 0);

		MS_DEBUG_DEV("new available bitrate:%" PRIi64, this->bitrates.availableBitrate);

		MayEmitAvailableBitrateEvent(previousAvailableBitrate);
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RTP::Packet* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);
	}

	RTC::RTP::Packet* TransportCongestionControlClient::GeneratePadding(size_t size)
	{
		MS_TRACE();
		MS_ASSERT(this->probationGenerator, "probation generator not initialized")

		return this->probationGenerator->GetNextPacket(size);
	}

	void TransportCongestionControlClient::OnTimer(TimerHandleInterface* timer)
	{
		MS_TRACE();

		if (timer == this->processTimer)
		{
			// Time to call RtpTransportControllerSend::Process().
			this->rtpTransportControllerSend->Process();

			// Time to call PacedSender::Process().
			this->rtpTransportControllerSend->packet_sender()->Process();

			this->processTimer->Start(
			  std::min<uint64_t>(
			    // Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
			    this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
			    // Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
			    this->controllerFactory->GetProcessInterval().ms()));

			MayEmitAvailableBitrateEvent(this->bitrates.availableBitrate);
		}
	}
} // namespace RTC
