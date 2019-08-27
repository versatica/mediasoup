#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV

#include "RTC/TransportCongestionControlClient.hpp"
#include "Logger.hpp"

static std::unique_ptr<webrtc::NetworkStatePredictorFactoryInterface> predictorFactory{ nullptr };
static std::unique_ptr<webrtc::NetworkControllerFactoryInterface> controllerFactory{ nullptr };

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
		}

		this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
		  this, predictorFactory.get(), controllerFactory.get(), bitrateConfig);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		// TODO: timers?
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		// TODO: timers?
		delete this->rtpTransportControllerSend;
		this->rtpTransportControllerSend = nullptr;
	}

	void TransportCongestionControlClient::InsertPacket(
	  uint32_t ssrc, uint16_t sequenceNumber, int64_t captureTimeMs, size_t bytes, bool retransmission)
	{
		this->rtpTransportControllerSend->packet_sender()->InsertPacket(
		  ssrc, sequenceNumber, captureTimeMs, bytes, retransmission);
	}

	void TransportCongestionControlClient::TransportConnected()
	{
		MS_TRACE();

		// TODO: timers?

		this->rtpTransportControllerSend->OnNetworkAvailability(true);
	}

	void TransportCongestionControlClient::TransportDisconnected()
	{
		MS_TRACE();

		// TODO: timers?

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

	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& cluster_info)
	{
		MS_TRACE();

		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet);
	}

	// TODO:
	// Should generate probation packets.
	std::vector<RTC::RtpPacket*> TransportCongestionControlClient::GeneratePadding(size_t target_size_bytes)
	{
		MS_TRACE();

		return {};
	}
} // namespace RTC
