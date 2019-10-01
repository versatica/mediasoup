#define MS_CLASS "RTC::TransportCongestionControlClient"
#define MS_LOG_DEV // TODO

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "system_wrappers/source/field_trial.h" // webrtc::field_trial
#include <limits>

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.
	// TODO: Look for a proper value.
	static const char FieldTrials[]{ "WebRTC-Pacer-MinPacketLimitMs/Enabled,100/" };

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener,
	  RTC::BweType bweType,
	  uint32_t initialAvailableBitrate)
	  : listener(listener), bweType(bweType), initialAvailableBitrate(initialAvailableBitrate)
	{
		MS_TRACE();

		// TODO: Add into ClassInit static member?
		webrtc::field_trial::InitFieldTrialsFromString(FieldTrials);
		// TODO: Must these factories be static members?

		// TODO: Create predictor factory?

		// TODO: Create controller factory. Let's see.
		webrtc::GoogCcFactoryConfig config;

		config.feedback_only = bweType == RTC::BweType::TRANSPORT_CC;

		this->controllerFactory = new webrtc::GoogCcNetworkControllerFactory(std::move(config));

		webrtc::BitrateConstraints bitrateConfig;

		bitrateConfig.start_bitrate_bps = static_cast<int>(this->initialAvailableBitrate);

		this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
		  this, this->predictorFactory, this->controllerFactory, bitrateConfig);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->probationGenerator = new RTC::RtpProbationGenerator();

		this->processTimer = new Timer(this);

		/* clang-format off */
		this->processTimer->Start(std::min(
			// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
			this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
			// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
			this->controllerFactory->GetProcessInterval().ms()
		));
		/* clang-format on */
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		delete this->predictorFactory;
		this->predictorFactory = nullptr;

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

		auto now = DepLibUV::GetTimeMs();

		this->desiredBitrateTrend.ForceUpdate(0u, now);
		this->rtpTransportControllerSend->OnNetworkAvailability(false);
	}

	void TransportCongestionControlClient::InsertPacket(webrtc::RtpPacketSendInfo& packetInfo)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->packet_sender()->InsertPacket(packetInfo.length);
		this->rtpTransportControllerSend->OnAddPacket(packetInfo);

		// // TODO: Testing
		// MS_WARN_DEV("---- delay:%lld" PRIu64,
		// this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());
	}

	webrtc::PacedPacketInfo TransportCongestionControlClient::GetPacingInfo()
	{
		MS_TRACE();

		return this->rtpTransportControllerSend->packet_sender()->GetPacingInfo();
	}

	void TransportCongestionControlClient::PacketSent(webrtc::RtpPacketSendInfo& packetInfo, uint64_t now)
	{
		MS_TRACE();

		// Notify the transport feedback adapter about the sent packet.
		rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, now);
		this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);

		// // TODO: Testing
		// MS_WARN_DEV("---- delay:%lld" PRIu64,
		// this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());
	}

	void TransportCongestionControlClient::ReceiveEstimatedBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		// TODO: REMOVE
		MS_DEBUG_DEV("--- REMB bitrate:%" PRIu32, bitrate);

		this->rtpTransportControllerSend->OnReceivedEstimatedBitrate(bitrate);
	}

	void TransportCongestionControlClient::ReceiveRtcpReceiverReport(
	  const webrtc::RTCPReportBlock& report, float rtt, uint64_t now)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnReceivedRtcpReceiverReport(
		  { report }, static_cast<int64_t>(rtt), static_cast<int64_t>(now));
	}

	void TransportCongestionControlClient::ReceiveRtcpTransportFeedback(
	  const RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnTransportFeedback(*feedback);

		// // TODO: Testing
		// MS_WARN_DEV("---- delay:%lld" PRIu64,
		// this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());
	}

	void TransportCongestionControlClient::SetDesiredBitrate(uint32_t desiredBitrate, bool force)
	{
		MS_TRACE();

		auto now = DepLibUV::GetTimeMs();
		uint32_t minBitrate{ 50000u };
		uint32_t maxBitrate;
		uint32_t maxPaddingBitrate;

		// Manage it via trending and increase it a bit to avoid immediate oscillations.
		if (!force)
			this->desiredBitrateTrend.Update(desiredBitrate, now);
		else
			this->desiredBitrateTrend.ForceUpdate(desiredBitrate, now);

		maxBitrate = std::max<uint32_t>(minBitrate, this->desiredBitrateTrend.GetValue() * 1.25);

		if (this->desiredBitrateTrend.GetValue() != 0u)
			maxPaddingBitrate = maxBitrate * 0.75;
		else
			maxPaddingBitrate = minBitrate * 0.25;

		MS_DEBUG_DEV(
		  "[desiredBitrate:%" PRIu32 ", maxBitrate:%" PRIu32 ", maxPaddingBitrate:%" PRIu32 "]",
		  desiredBitrate,
		  maxBitrate,
		  maxPaddingBitrate);

		this->rtpTransportControllerSend->SetAllocatedSendBitrateLimits(
		  minBitrate, maxPaddingBitrate, maxBitrate);

		// // TODO: Testing
		// MS_WARN_DEV("---- delay:%lld" PRIu64,
		// this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());
	}

	uint32_t TransportCongestionControlClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->availableBitrate;
	}

	void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAt = DepLibUV::GetTimeMs();
	}

	void TransportCongestionControlClient::MayEmitAvailableBitrateEvent(uint32_t previousAvailableBitrate)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTimeMs();
		bool notify{ false };

		// Ignore if first event.
		// NOTE: Otherwise it will make the Transport crash since this event also happens
		// during the constructor of this class.
		if (this->lastAvailableBitrateEventAt == 0u)
		{
			this->lastAvailableBitrateEventAt = now;

			return;
		}

		// Emit if this is the first valid event.
		if (!this->availableBitrateEventCalled)
		{
			this->availableBitrateEventCalled = true;

			notify = true;
		}
		// Emit event if AvailableBitrateEventInterval elapsed.
		else if (now - this->lastAvailableBitrateEventAt >= AvailableBitrateEventInterval)
		{
			notify = true;
		}
		// Also emit the event fast if we detect a high BWE value decrease.
		else if (this->availableBitrate < previousAvailableBitrate * 0.75)
		{
			MS_WARN_TAG(
			  bwe,
			  "high BWE value decrease detected, notifying the listener [now:%" PRIu32 ", before:%" PRIu32
			  "]",
			  this->availableBitrate,
			  previousAvailableBitrate);

			notify = true;
		}

		if (notify)
		{
			MS_DEBUG_DEV(
			  "notifying the listener with new available bitrate:%" PRIu32, this->availableBitrate);

			this->lastAvailableBitrateEventAt = now;

			this->listener->OnTransportCongestionControlClientAvailableBitrate(
			  this, this->availableBitrate, previousAvailableBitrate);
		}
	}

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;

		// Update availableBitrate.
		// NOTE: Just in case.
		if (targetTransferRate.target_rate.bps() > std::numeric_limits<uint32_t>::max())
			this->availableBitrate = std::numeric_limits<uint32_t>::max();
		else
			this->availableBitrate = static_cast<uint32_t>(targetTransferRate.target_rate.bps());

		// MS_DEBUG_DEV("new available bitrate:%" PRIu32, this->availableBitrate);

		MayEmitAvailableBitrateEvent(previousAvailableBitrate);
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);

		// // TODO: Testing
		// MS_WARN_DEV("---- delay:%lld" PRIu64,
		// this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());
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
			this->processTimer->Start(std::min(
				// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
				this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
				// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
				this->controllerFactory->GetProcessInterval().ms()
			));
			/* clang-format on */

			// TODO: REMOVE
			// MS_DEBUG_DEV("---- delay:%" PRIu64, this->processTimer->GetTimeout());

			MayEmitAvailableBitrateEvent(this->availableBitrate);
		}
	}
} // namespace RTC
