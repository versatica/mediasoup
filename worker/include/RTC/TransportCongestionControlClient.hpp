#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/BweType.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/ProbationGenerator.hpp"
#include "RTC/TrendCalculator.hpp"
#include "SharedInterface.hpp"
#include <libwebrtc/api/transport/goog_cc_factory.h>
#include <libwebrtc/api/transport/network_types.h>
#include <libwebrtc/call/rtp_transport_controller_send.h>
#include <libwebrtc/modules/pacing/packet_router.h>
#include <deque>

namespace RTC
{
	class TransportCongestionControlClient : public webrtc::PacketRouter,
	                                         public webrtc::TargetTransferRateObserver,
	                                         public TimerHandleInterface::Listener
	{
	public:
		struct Bitrates
		{
			int64_t desiredBitrate{ 0 };
			int64_t effectiveDesiredBitrate{ 0 };
			int64_t minBitrate{ 0 };
			int64_t maxBitrate{ 0 };
			int64_t startBitrate{ 0 };
			int64_t maxPaddingBitrate{ 0 };
			int64_t availableBitrate{ 0 };
		};

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTransportCongestionControlClientBitrates(
			  RTC::TransportCongestionControlClient* tccClient,
			  RTC::TransportCongestionControlClient::Bitrates& bitrates) = 0;
			virtual void OnTransportCongestionControlClientSendRtpPacket(
			  RTC::TransportCongestionControlClient* tccClient,
			  RTC::RTP::Packet* packet,
			  const webrtc::PacedPacketInfo& pacingInfo) = 0;
		};

	public:
		/**
		 * @param absoluteMinOutgoingBitrate - Bitrate the target is never taken
		 *   below (bps), whatever the API asks for.
		 */
		TransportCongestionControlClient(
		  RTC::TransportCongestionControlClient::Listener* listener,
		  SharedInterface* shared,
		  RTC::BweType bweType,
		  int64_t absoluteMinOutgoingBitrate,
		  int64_t initialAvailableBitrate,
		  int64_t maxOutgoingBitrate,
		  int64_t minOutgoingBitrate);
		~TransportCongestionControlClient() override;

	public:
		RTC::BweType GetBweType() const
		{
			return this->bweType;
		}
		void TransportConnected();
		void TransportDisconnected();
		void InsertPacket(webrtc::RtpPacketSendInfo& packetInfo);
		webrtc::PacedPacketInfo GetPacingInfo();
		void PacketSent(const webrtc::RtpPacketSendInfo& packetInfo, int64_t nowUs);
		void ReceiveEstimatedBitrate(int64_t bitrate);
		void ReceiveRtcpReceiverReport(
		  RTC::RTCP::ReceiverReportPacket* packet, float rttMs, int64_t receivedAtUs);
		void ReceiveRtcpTransportFeedback(const RTC::RTCP::FeedbackRtpTransportPacket* feedback);
		void SetDesiredBitrate(int64_t desiredBitrate, bool force);
		void SetMaxOutgoingBitrate(int64_t maxBitrate);
		void SetMinOutgoingBitrate(int64_t minBitrate);
		const Bitrates& GetBitrates() const
		{
			return this->bitrates;
		}
		int64_t GetAvailableBitrate() const;
		double GetPacketLoss() const;
		void RescheduleNextAvailableBitrateEvent();

	private:
		void MayEmitAvailableBitrateEvent(int64_t previousAvailableBitrate);
		void UpdatePacketLoss(double packetLoss);
		void ApplyBitrateUpdates();

		void InitializeController();
		void DestroyController();

		// jmillan: missing.
		// void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

		/* Pure virtual methods inherited from webrtc::TargetTransferRateObserver. */
	public:
		void OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate) override;

		/* Pure virtual methods inherited from webrtc::PacketRouter. */
	public:
		void SendPacket(RTC::RTP::Packet* packet, const webrtc::PacedPacketInfo& pacingInfo) override;
		RTC::RTP::Packet* GeneratePadding(size_t size) override;

		/* Pure virtual methods inherited from RTC::TimerHandleInterface. */
	public:
		void OnTimer(TimerHandleInterface* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		SharedInterface* shared{ nullptr };
		// Allocated by this.
		webrtc::NetworkControllerFactoryInterface* controllerFactory{ nullptr };
		webrtc::RtpTransportControllerSend* rtpTransportControllerSend{ nullptr };
		RTC::RTP::ProbationGenerator* probationGenerator{ nullptr };
		TimerHandleInterface* processTimer{ nullptr };
		// Others.
		const RTC::BweType bweType;
		const int64_t absoluteMinOutgoingBitrate;
		const int64_t initialAvailableBitrate;
		int64_t maxOutgoingBitrate{ 0 };
		int64_t minOutgoingBitrate{ 0 };
		Bitrates bitrates;
		bool availableBitrateEventCalled{ false };
		int64_t lastAvailableBitrateEventAtMs{ 0 };
		RTC::TrendCalculator desiredBitrateTrend;
		std::deque<double> packetLossHistory;
		double packetLoss{ 0 };
	};
} // namespace RTC

#endif
