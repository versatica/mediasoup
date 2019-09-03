#define MS_CLASS "RTC::TransportCongestionControlClient"
#define MS_LOG_DEV // TODO

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <limits>

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.
	// TODO: No. Must be dynamic.
	// Size of probation packets.
	static constexpr size_t ProbationPacketSize{ 250u };

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener,
	  BweType bweType,
	  uint32_t initialAvailableBitrate)
	  : listener(listener)
	{
		MS_TRACE();

		// TODO: Must these factories be static members?

		// TODO: Create predictor factory?

		// TODO: Create controller factory. Let's see.
		webrtc::GoogCcFactoryConfig config;

		config.feedback_only = bweType == BweType::TRANSPORT_WIDE_CONGESTION;

		this->controllerFactory = new webrtc::GoogCcNetworkControllerFactory(std::move(config));

		webrtc::BitrateConstraints bitrateConfig;

		bitrateConfig.start_bitrate_bps = static_cast<int>(initialAvailableBitrate);

		this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
		  this, this->predictorFactory, this->controllerFactory, bitrateConfig);

		this->probationGenerator = new RTC::RtpProbationGenerator(ProbationPacketSize);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->pacerTimer = new Timer(this);

		auto delay = static_cast<uint64_t>(
		  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());

		this->pacerTimer->Start(delay);
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

		delete this->pacerTimer;
		this->pacerTimer = nullptr;
	}

	void TransportCongestionControlClient::InsertPacket(size_t bytes)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->packet_sender()->InsertPacket(bytes);
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
		this->rtpTransportControllerSend->OnAddPacket(packetInfo);

		// Notify the transport feedback adapter about the sent packet.
		rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, now);
		this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);
	}

	void TransportCongestionControlClient::TransportConnected()
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnNetworkAvailability(true);
	}

	void TransportCongestionControlClient::TransportDisconnected()
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnNetworkAvailability(false);
	}

	void TransportCongestionControlClient::ReceiveEstimatedBitrate(uint32_t bitrate)
	{
		MS_TRACE();

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
	}

	void TransportCongestionControlClient::SetDesiredBitrates(
	  int minSendBitrateBps, int maxPaddingBitrateBps, int maxTotalBitrateBps)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->SetAllocatedSendBitrateLimits(
		  minSendBitrateBps, maxPaddingBitrateBps, maxTotalBitrateBps);
	}

	uint32_t TransportCongestionControlClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->availableBitrate;
	}

	void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAt = DepLibUV::GetTime();
	}

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		auto previousAvailableBitrate = this->availableBitrate;
		uint64_t now                  = DepLibUV::GetTime();

		// Update availableBitrate.
		// NOTE: Just in case.
		if (targetTransferRate.target_rate.bps() > std::numeric_limits<uint32_t>::max())
			this->availableBitrate = std::numeric_limits<uint32_t>::max();
		else
			this->availableBitrate = static_cast<uint32_t>(targetTransferRate.target_rate.bps());

		// TODO: This produces lot of logs with the very same availableBitrate, so why is this
		// event called so frequently?
		MS_DEBUG_DEV("new availableBitrate:%" PRIu32, this->availableBitrate);

		// Ignore if first event.
		// NOTE: Otherwise it will make the Transport crash since this event also happens
		// during the constructor of this class.
		if (this->lastAvailableBitrateEventAt == 0u)
		{
			this->lastAvailableBitrateEventAt = now;

			return;
		}

		// Emit event if AvailableBitrateEventInterval elapsed.
		if (now - this->lastAvailableBitrateEventAt >= AvailableBitrateEventInterval)
		{
			this->lastAvailableBitrateEventAt = now;

			this->listener->OnTransportCongestionControlClientAvailableBitrate(
			  this, this->availableBitrate, previousAvailableBitrate);
		}
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);
	}

	std::vector<RTC::RtpPacket*> TransportCongestionControlClient::GeneratePadding(size_t /*size*/)
	{
		MS_TRACE();

		// TODO: Must generate a packet of the requested size.
		return { this->probationGenerator->GetNextPacket() };
	}

	void TransportCongestionControlClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->pacerTimer)
		{
			// Time to call PacedSender::Process().
			this->rtpTransportControllerSend->packet_sender()->Process();

			auto delay = static_cast<uint64_t>(
			  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess());

			this->pacerTimer->Start(delay);
		}
	}
} // namespace RTC
