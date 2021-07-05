#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include "common.hpp"
#include "RTC/BweType.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h>

namespace RTC
{
	class TransportCongestionControlServer
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

    struct Strategy {
      virtual ~Strategy() {}
      virtual uint32_t GetAvailableBitrate() { return 0u; }
      virtual void TransportConnected() {}
      virtual void TransportDisconnected() {}
      virtual void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet) = 0;

      void SetMaxIncomingBitrate(uint32_t bitrate);
      void MaySendLimitationRembFeedback();

			virtual bool NeedSendREMB() const = 0;

      static std::unique_ptr<TransportCongestionControlServer::Strategy> Create(
			  TransportCongestionControlServer* parent,
			  TransportCongestionControlServer::Listener* listener,
			  BweType bweType,
			  size_t maxRtcpPacketLen);

      uint32_t maxIncomingBitrate{ 0u };
      uint64_t limitationRembSentAtMs{ 0u };

      uint8_t unlimitedRembCounter{ 0u };
      TransportCongestionControlServer::Listener* listener{ nullptr };
      TransportCongestionControlServer* parent{ nullptr };
      size_t maxRtcpPacketLen{ 0u };

		};
		
  public:
		TransportCongestionControlServer(
		  RTC::TransportCongestionControlServer::Listener* listener,
		  RTC::BweType bweType,
		  size_t maxRtcpPacketLen);
		virtual ~TransportCongestionControlServer();

	public:
		void TransportConnected();
		void TransportDisconnected();
		uint32_t GetAvailableBitrate() const
		{
			return controller->GetAvailableBitrate();
		}
		void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet);
		void SetMaxIncomingBitrate(uint32_t bitrate);

	private:
    std::unique_ptr<Strategy> controller;
	};
} // namespace RTC

#endif
