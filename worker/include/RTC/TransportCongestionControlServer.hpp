#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	class TransportCongestionControlServer : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportCongestionControlServerSendRtcpPacket(
			  RTC::TransportCongestionControlServer* tccServer, RTC::RTCP::Packet* packet) = 0;
		};

	public:
		TransportCongestionControlServer(
		  RTC::TransportCongestionControlServer::Listener* listener, size_t maxRtcpPacketLen);
		virtual ~TransportCongestionControlServer();

	public:
		void TransportConnected();
		void TransportDisconnected();
		void IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber);

	private:
		void SendFeedback();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		Timer* feedbackSendPeriodicTimer{ nullptr };
		std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket> feedbackPacket;
		// Others.
		size_t maxRtcpPacketLen{ 0u };
		uint8_t feedbackPacketCount{ 0u };
	};
} // namespace RTC

#endif
