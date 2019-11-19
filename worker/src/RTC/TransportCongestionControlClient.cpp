#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <libwebrtc/api/transport/network_types.h> // webrtc::TargetRateConstraints
#include <limits>

namespace RTC
{
	/* Static. */

	static constexpr uint32_t MinBitrate{ 30000u };
	static constexpr float MaxBitrateIncrementFactor{ 1.35f };
	static constexpr float MaxPaddingBitrateFactor{ 0.85f };
	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener,
	  RTC::BweType bweType,
	  uint32_t initialAvailableBitrate)
	  : listener(listener), bweType(bweType),
	    initialAvailableBitrate(std::max<uint32_t>(initialAvailableBitrate, MinBitrate))
	{
		MS_TRACE();

		webrtc::GoogCcFactoryConfig config;

		config.feedback_only = bweType == RTC::BweType::TRANSPORT_CC;

		this->controllerFactory = new webrtc::GoogCcNetworkControllerFactory(std::move(config));

		webrtc::BitrateConstraints bitrateConfig;

		bitrateConfig.start_bitrate_bps = static_cast<int>(this->initialAvailableBitrate);

		this->rtpTransportControllerSend =
		  new webrtc::RtpTransportControllerSend(this, nullptr, this->controllerFactory, bitrateConfig);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->probationGenerator = new RTC::RtpProbationGenerator();

		this->processTimer = new Timer(this);

		// TODO: Let's see.
		// this->rtpTransportControllerSend->EnablePeriodicAlrProbing(true);

		// clang-format off
		this->processTimer->Start(std::min(
			// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
			this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
			// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
			this->controllerFactory->GetProcessInterval().ms()
		));
		// clang-format on
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		delete this->controllerFactory;
		this->controllerFactory = nullptr;

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

		this->rtpTransportControllerSend->OnNetworkAvailability(true);
	}

	void TransportCongestionControlClient::TransportDisconnected()
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMs();

		this->bitrates.desiredBitrate          = 0u;
		this->bitrates.effectiveDesiredBitrate = 0u;

		this->desiredBitrateTrend.ForceUpdate(0u, nowMs);
		this->rtpTransportControllerSend->OnNetworkAvailability(false);
	}

	void TransportCongestionControlClient::InsertPacket(webrtc::RtpPacketSendInfo& packetInfo)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->packet_sender()->InsertPacket(packetInfo.length);
		this->rtpTransportControllerSend->OnAddPacket(packetInfo);
	}

	webrtc::PacedPacketInfo TransportCongestionControlClient::GetPacingInfo()
	{
		MS_TRACE();

		return this->rtpTransportControllerSend->packet_sender()->GetPacingInfo();
	}

	void TransportCongestionControlClient::PacketSent(webrtc::RtpPacketSendInfo& packetInfo, uint64_t nowMs)
	{
		MS_TRACE();

		// Notify the transport feedback adapter about the sent packet.
		rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, nowMs);
		this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);
	}

	void TransportCongestionControlClient::ReceiveEstimatedBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnReceivedEstimatedBitrate(bitrate);
	}

	void TransportCongestionControlClient::ReceiveRtcpReceiverReport(
	  const webrtc::RTCPReportBlock& report, float rtt, uint64_t nowMs)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnReceivedRtcpReceiverReport(
		  { report }, static_cast<int64_t>(rtt), static_cast<int64_t>(nowMs));
	}

	void TransportCongestionControlClient::ReceiveRtcpTransportFeedback(
	  const RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnTransportFeedback(*feedback);
	}

	void TransportCongestionControlClient::SetDesiredBitrate(uint32_t desiredBitrate, bool force)
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMs();

		// Manage it via trending and increase it a bit to avoid immediate oscillations.
		if (!force)
			this->desiredBitrateTrend.Update(desiredBitrate, nowMs);
		else
			this->desiredBitrateTrend.ForceUpdate(desiredBitrate, nowMs);

		this->bitrates.desiredBitrate          = desiredBitrate;
		this->bitrates.effectiveDesiredBitrate = this->desiredBitrateTrend.GetValue();
		this->bitrates.minBitrate              = MinBitrate;

		if (this->desiredBitrateTrend.GetValue() > 0u)
		{
			this->bitrates.maxBitrate = std::max<uint32_t>(
			  this->initialAvailableBitrate,
			  this->desiredBitrateTrend.GetValue() * MaxBitrateIncrementFactor);
			this->bitrates.startBitrate = std::min<uint32_t>(
			  this->bitrates.maxBitrate,
			  std::max<uint32_t>(this->bitrates.availableBitrate, this->initialAvailableBitrate));
			this->bitrates.maxPaddingBitrate = this->bitrates.maxBitrate * MaxPaddingBitrateFactor;
		}
		else
		{
			this->bitrates.maxBitrate        = this->initialAvailableBitrate;
			this->bitrates.startBitrate      = this->initialAvailableBitrate;
			this->bitrates.maxPaddingBitrate = 0u;
		}

		MS_DEBUG_DEV(
		  "[desiredBitrate:%" PRIu32 ", startBitrate:%" PRIu32 ", minBitrate:%" PRIu32
		  ", maxBitrate:%" PRIu32 ", maxPaddingBitrate:%" PRIu32 "]",
		  this->bitrates.desiredBitrate,
		  this->bitrates.startBitrate,
		  this->bitrates.minBitrate,
		  this->bitrates.maxBitrate,
		  this->bitrates.maxPaddingBitrate);

		this->rtpTransportControllerSend->SetAllocatedSendBitrateLimits(
		  this->bitrates.minBitrate, this->bitrates.maxPaddingBitrate, this->bitrates.maxBitrate);

		webrtc::TargetRateConstraints constraints;

		constraints.at_time       = webrtc::Timestamp::ms(DepLibUV::GetTimeMs());
		constraints.min_data_rate = webrtc::DataRate::bps(this->bitrates.minBitrate);
		constraints.max_data_rate = webrtc::DataRate::bps(this->bitrates.maxBitrate);
		constraints.starting_rate = webrtc::DataRate::bps(this->bitrates.startBitrate);

		this->rtpTransportControllerSend->SetClientBitratePreferences(constraints);
	}

	uint32_t TransportCongestionControlClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->bitrates.availableBitrate;
	}

	void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAtMs = DepLibUV::GetTimeMs();
	}

	void TransportCongestionControlClient::MayEmitAvailableBitrateEvent(uint32_t previousAvailableBitrate)
	{
		MS_TRACE();

		uint64_t nowMs = DepLibUV::GetTimeMs();
		bool notify{ false };

		// Ignore if first event.
		// NOTE: Otherwise it will make the Transport crash since this event also happens
		// during the constructor of this class.
		if (this->lastAvailableBitrateEventAtMs == 0u)
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
		// Emit event if AvailableBitrateEventInterval elapsed.
		else if (nowMs - this->lastAvailableBitrateEventAtMs >= AvailableBitrateEventInterval)
		{
			notify = true;
		}
		// Also emit the event fast if we detect a high BWE value decrease.
		else if (this->bitrates.availableBitrate < previousAvailableBitrate * 0.75)
		{
			MS_WARN_TAG(
			  bwe,
			  "high BWE value decrease detected, notifying the listener [now:%" PRIu32 ", before:%" PRIu32
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
			  "high BWE value increase detected, notifying the listener [now:%" PRIu32 ", before:%" PRIu32
			  "]",
			  this->bitrates.availableBitrate,
			  previousAvailableBitrate);

			notify = true;
		}

		if (notify)
		{
			MS_DEBUG_DEV(
			  "notifying the listener with new available bitrate:%" PRIu32,
			  this->bitrates.availableBitrate);

			this->lastAvailableBitrateEventAtMs = nowMs;

			this->listener->OnTransportCongestionControlClientBitrates(this, this->bitrates);
		}
	}

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->bitrates.availableBitrate;

		// Update availableBitrate.
		// NOTE: Just in case.
		if (targetTransferRate.target_rate.bps() > std::numeric_limits<uint32_t>::max())
			this->bitrates.availableBitrate = std::numeric_limits<uint32_t>::max();
		else
			this->bitrates.availableBitrate = static_cast<uint32_t>(targetTransferRate.target_rate.bps());

		MS_DEBUG_DEV("new available bitrate:%" PRIu32, this->bitrates.availableBitrate);

		MayEmitAvailableBitrateEvent(previousAvailableBitrate);
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);
	}

	RTC::RtpPacket* TransportCongestionControlClient::GeneratePadding(size_t size)
	{
		MS_TRACE();

		return this->probationGenerator->GetNextPacket(size);
	}

	void TransportCongestionControlClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->processTimer)
		{
			// Time to call RtpTransportControllerSend::Process().
			this->rtpTransportControllerSend->Process();

			// Time to call PacedSender::Process().
			this->rtpTransportControllerSend->packet_sender()->Process();

			/* clang-format off */
			this->processTimer->Start(std::min<uint64_t>(
				// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
				this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
				// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
				this->controllerFactory->GetProcessInterval().ms()
			));
			/* clang-format on */

			MayEmitAvailableBitrateEvent(this->bitrates.availableBitrate);
		}
	}
} // namespace RTC
