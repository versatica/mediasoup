#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpProbationGenerator.hpp"
#include "RTC/SendTransportController/goog_cc_factory.h"
#include "RTC/SendTransportController/network_types.h" // TargetTransferRate
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
		enum class BweType
		{
			TRANSPORT_WIDE_CONGESTION = 1,
			REMB
		};

	public:
		class Listener
		{
		public:
			virtual void OnTransportCongestionControlClientAvailableBitrate(
			  RTC::TransportCongestionControlClient* tccClient,
			  uint32_t availableBitrate,
			  uint32_t previousAvailableBitrate) = 0;

			virtual void OnTransportCongestionControlClientSendRtpPacket(
			  RTC::TransportCongestionControlClient* tccClient,
			  RTC::RtpPacket* packet,
			  const webrtc::PacedPacketInfo& pacingInfo) = 0;
		};

	public:
		TransportCongestionControlClient(
		  RTC::TransportCongestionControlClient::Listener* listener,
		  BweType bweType,
		  uint32_t initialAvailableBitrate);
		virtual ~TransportCongestionControlClient();

	public:
		void InsertPacket(size_t bytes);
		webrtc::PacedPacketInfo GetPacingInfo();
		void PacketSent(webrtc::RtpPacketSendInfo& packetInfo, uint64_t now);
		void TransportConnected();
		void TransportDisconnected();
		void ReceiveEstimatedBitrate(uint32_t bitrate);
		void ReceiveRtcpReceiverReport(const webrtc::RTCPReportBlock& report, float rtt, uint64_t now);
		void ReceiveRtcpTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket* feedback);
		void SetDesiredBitrates(int minSendBitrateBps, int maxPaddingBitrateBps, int maxTotalBitrateBps);
		uint32_t GetAvailableBitrate() const;
		void RescheduleNextAvailableBitrateEvent();

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
		webrtc::NetworkStatePredictorFactoryInterface* predictorFactory{ nullptr };
		webrtc::NetworkControllerFactoryInterface* controllerFactory{ nullptr };
		webrtc::RtpTransportControllerSend* rtpTransportControllerSend{ nullptr };
		RTC::RtpProbationGenerator* probationGenerator{ nullptr };
		Timer* pacerTimer{ nullptr };
		// Others.
		uint32_t availableBitrate{ 0 };
		uint64_t lastAvailableBitrateEventAt{ 0 };
	};
} // namespace RTC

#endif
