#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/SendTransportController/goog_cc_factory.h"

static std::unique_ptr<webrtc::NetworkStatePredictorFactoryInterface> predictorFactory{ nullptr };
static std::unique_ptr<webrtc::NetworkControllerFactoryInterface> controllerFactory{ nullptr };

// Size of probation packets.
constexpr size_t probationPacketSize{ 250 };

// jmillan: TODO.
webrtc::BitrateConstraints bitrateConfig;

namespace RTC
{
	/* Static. */

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener)
	  : listener(listener)
	{
		MS_TRACE();

		// TODO: Create predictor factory.
		if (!predictorFactory)
		{
		}

		// TODO: Create controller factory.
		if (!controllerFactory)
		{
			webrtc::GoogCcFactoryConfig config;
			config.feedback_only = true;
			controllerFactory.reset(new webrtc::GoogCcNetworkControllerFactory(std::move(config)));
		}

		bitrateConfig.start_bitrate_bps = 500000;
		this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
		  this, predictorFactory.get(), controllerFactory.get(), bitrateConfig);

		this->probationGenerator = new RTC::RtpProbationGenerator(probationPacketSize);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->pacerTimer = new Timer(this);

		auto TimeUntilNextProcess =
		  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess();

		this->pacerTimer->Start(TimeUntilNextProcess);
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		delete this->rtpTransportControllerSend;
		this->rtpTransportControllerSend = nullptr;

		delete this->probationGenerator;
		this->probationGenerator = nullptr;

		if (this->pacerTimer)
		{
			this->pacerTimer->Stop();
			delete this->pacerTimer;
		}
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

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		this->listener->OnTransportCongestionControlClientTargetTransferRate(this, targetTransferRate);
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

		return { this->probationGenerator->GetNextPacket() };
	}

	void TransportCongestionControlClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->pacerTimer)
		{
			// Time to call PacedSender::Process().
			this->rtpTransportControllerSend->packet_sender()->Process();

			auto TimeUntilNextProcess =
			  this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess();

			this->pacerTimer->Start(TimeUntilNextProcess);
		}
	}
} // namespace RTC
