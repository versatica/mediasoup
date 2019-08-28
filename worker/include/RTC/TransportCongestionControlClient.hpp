#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/SendTransportController/network_types.h" // TargetTransferRate.
#include "RTC/SendTransportController/pacing/packet_router.h"
#include "RTC/SendTransportController/rtp_transport_controller_send.h"
#include "handles/Timer.hpp"

namespace RTC
{
	class TransportCongestionControlClient : public webrtc::PacketRouter,
	                                         public webrtc::TargetTransferRateObserver,
	                                         public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportCongestionControlClientTargetTransferRate(
			  RTC::TransportCongestionControlClient* tccClient,
			  webrtc::TargetTransferRate targetTransferRate) = 0;

			virtual void OnTransportCongestionControlClientSendRtpPacket(
			  RTC::TransportCongestionControlClient* tccClient, RTC::RtpPacket* packet) = 0;
		};

	public:
		TransportCongestionControlClient(RTC::TransportCongestionControlClient::Listener* listener);
		virtual ~TransportCongestionControlClient();

	public:
		void InsertPacket(size_t bytes);
		void PacketSent(rtc::SentPacket& sentPacket);
		void TransportConnected();
		void TransportDisconnected();
		void ReceiveEstimatedBitrate(uint32_t bitrate);
		void ReceiveRtcpReceiverReport(const webrtc::RTCPReportBlock& report, int64_t rtt, int64_t now_ms);
		void ReceiveRtcpTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket* feedback);

		// jmillan: missing.
		// void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

		/* Pure virtual methods inherited from webrtc::TargetTransferRateObserver. */
	public:
		void OnTargetTransferRate(webrtc::TargetTransferRate targetTranferRate) override;

		/* Pure virtual methods inherited from webrtc::PacketRouter. */
	public:
		virtual void SendPacket(RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo) override;

		virtual std::vector<RTC::RtpPacket*> GeneratePadding(size_t size) override;

		/* Pure virtual methods inherited from RTC::Timer. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		webrtc::RtpTransportControllerSend* rtpTransportControllerSend{ nullptr };
		Timer* pacerTimer{ nullptr };
	};
} // namespace RTC

#endif
