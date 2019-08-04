#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include "common.hpp"
// TODO
// #include "RTC/RTCP/FeedbackRtpTransport.hpp"

namespace RTC
{
	class TransportCongestionControlServer
	{
	public:
		class Listener
		{
		public:
			// TODO
			// virtual void OnTransportCongestionControlServerSendFeedback(
			//   RTC::TransportCongestionControlServer* tccServer,
			//   RTC::RTCP::FeedbackRtpTransport* packet) = 0;
		};

	public:
		TransportCongestionControlServer(RTC::TransportCongestionControlServer::Listener* listener);
		virtual ~TransportCongestionControlServer();

	public:
		void IncomingPacket(int64_t arrivalTimeMs, uint16_t wideSeqNumber);

	private:
		void SendFeedback();

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Alloated by this.
		// TODO
		// RTC::RTCP::FeedbackRtpTransportPacket* transportPacket{ nullptr };
		// Others.
		uint64_t lastRtpPacketReceivedAt{ 0u };
	};
} // namespace RTC

#endif
