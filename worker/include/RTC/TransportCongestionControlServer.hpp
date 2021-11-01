#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include "common.hpp"
#include "RTC/BweType.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h>
#include <deque>

namespace RTC
{
	class TransportCongestionControlServer : public webrtc::RemoteBitrateEstimator::Listener,
	                                         public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnTransportCongestionControlServerSendRtcpPacket(
			  RTC::TransportCongestionControlServer* tccServer, RTC::RTCP::Packet* packet) = 0;
		};

	public:
		TransportCongestionControlServer(
		  RTC::TransportCongestionControlServer::Listener* listener,
		  RTC::BweType bweType,
		  size_t maxRtcpPacketLen);
		virtual ~TransportCongestionControlServer();

	public:
		RTC::BweType GetBweType() const
		{
			return this->bweType;
		}
		void TransportConnected();
		void TransportDisconnected();
		uint32_t GetAvailableBitrate() const
		{
			switch (this->bweType)
			{
				case RTC::BweType::REMB:
					return this->rembServer->GetAvailableBitrate();

				default:
					return 0u;
			}
		}
		double GetPacketLoss() const;
		void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet);
		void SetMaxIncomingBitrate(uint32_t bitrate);

	private:
		void SendTransportCcFeedback();
		void MaySendLimitationRembFeedback();
		void UpdatePacketLoss(double packetLoss);

		/* Pure virtual methods inherited from webrtc::RemoteBitrateEstimator::Listener. */
	public:
		void OnRembServerAvailableBitrate(
		  const webrtc::RemoteBitrateEstimator* remoteBitrateEstimator,
		  const std::vector<uint32_t>& ssrcs,
		  uint32_t availableBitrate) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		Timer* transportCcFeedbackSendPeriodicTimer{ nullptr };
		std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket> transportCcFeedbackPacket;
		webrtc::RemoteBitrateEstimatorAbsSendTime* rembServer{ nullptr };
		// Others.
		RTC::BweType bweType;
		size_t maxRtcpPacketLen{ 0u };
		uint8_t transportCcFeedbackPacketCount{ 0u };
		uint32_t transportCcFeedbackSenderSsrc{ 0u };
		uint32_t transportCcFeedbackMediaSsrc{ 0u };
		uint32_t maxIncomingBitrate{ 0u };
		uint64_t limitationRembSentAtMs{ 0u };
		uint8_t unlimitedRembCounter{ 0u };
		std::deque<double> packetLossHistory;
		double packetLoss{ 0 };
	};
} // namespace RTC

#endif
