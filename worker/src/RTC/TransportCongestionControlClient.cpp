#define MS_CLASS "RTC::TransportCongestionControlClient"
#define MS_LOG_DEV // TODO

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

// Size of probation packets.
static constexpr size_t ProbationPacketSize{ 250u };

// TMP: OnOutgoingAvailableBitrate fire module.
static uint8_t availableBitrateTrigger{ 4u };

namespace RTC
{
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

		// TODO: cast?
		bitrateConfig.start_bitrate_bps = initialAvailableBitrate;

		this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
		  this, this->predictorFactory, this->controllerFactory, bitrateConfig);

		this->probationGenerator = new RTC::RtpProbationGenerator(ProbationPacketSize);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->pacerTimer = new Timer(this);

		auto timeUntilNextProcess =
		  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess();

		  MS_DUMP("------ timeUntilNextProcess:%" PRIi64, timeUntilNextProcess);

		this->pacerTimer->Start(timeUntilNextProcess);

		this->initialized = true;
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

			MS_DEBUG_DEV("---- destructor starts");
			this->destroying = true;

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

			MS_DEBUG_DEV("---- destructor ends");
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

			MS_DUMP("------ jeje");

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
	  const webrtc::RTCPReportBlock& report, int64_t rtt, int64_t now_ms)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnReceivedRtcpReceiverReport({ report }, rtt, now_ms);
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

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		// TODO: Let's see.
		if (!this->initialized)
		{
			MS_ERROR("---- not yet initialized !!!");

			return;
		}

		// TODO: Keep this for debugging until it's clear that this never happens.
		if (this->destroying)
			MS_ERROR("---- called with this->destroying");

		// Trigger the callback once for each four times we get here.
		if (++availableBitrateTrigger % 4 == 0)
		{
			uint32_t previousAvailableBitrate = this->availableBitrate;
			this->availableBitrate            = targetTransferRate.target_rate.bps();

			this->listener->OnTransportCongestionControlClientAvailableBitrate(
			  this, this->availableBitrate, previousAvailableBitrate);
		}
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// TODO: Let's see.
		if (!this->initialized)
		{
			MS_ERROR("---- not yet initialized !!!");

			return;
		}

		// TODO: Keep this for debugging until it's clear that this never happens.
		if (this->destroying)
			MS_ERROR("---- called with this->destroying");

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);
	}

	std::vector<RTC::RtpPacket*> TransportCongestionControlClient::GeneratePadding(size_t /*size*/)
	{
		MS_TRACE();

		// TODO: Let's see.
		if (!this->initialized)
		{
			MS_ERROR("---- not yet initialized !!!");

			return {};
		}

		// TODO: Keep this for debugging until it's clear that this never happens.
		if (this->destroying)
			MS_ERROR("---- called with this->destroying");

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

			auto timeUntilNextProcess =
			  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess();

			this->pacerTimer->Start(timeUntilNextProcess);
		}
	}
} // namespace RTC
