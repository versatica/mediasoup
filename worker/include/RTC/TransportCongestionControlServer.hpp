#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/Packet.hpp"

namespace RTC
{
	class TransportCongestionControlServer
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
		void IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		// std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket> feedbackPacket;
		// Others.
		size_t maxRtcpPacketLen{ 0u };
		uint8_t feedbackPacketCount{ 0u };
		uint64_t lastRtpPacketReceivedAt{ 0u };
	};
} // namespace RTC

#endif
