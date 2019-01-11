#ifndef MS_RTC_PLAIN_RTP_TRANSPORT_HPP
#define MS_RTC_PLAIN_RTP_TRANSPORT_HPP

#include "common.hpp"
#include "RTC/Transport.hpp"

namespace RTC
{
	class PlainRtpTransport : public RTC::Transport
	{
	public:
		// TODO
		struct Options
		{
			std::string remoteIp;
			uint16_t remotePort;
			std::string localIp;
			bool preferIPv4;
			bool preferIPv6;
		};

	public:
		PlainRtpTransport(std::string& id, RTC::Transport::Listener* listener, Options& options);
		~PlainRtpTransport() override;

	public:
		virtual void Close();
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonObject) const override;
		void HandleRequest(Channel::Request* request) override;

	private:
		bool IsConnected() const override;
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;

		/* Private methods to unify UDP and TCP behavior. */
	private:
		void OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnPacketRecv(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

	private:
		// Allocated by this.
		RTC::UdpSocket* rtpUdpSocket{ nullptr };
		RTC::UdpSocket* rtcpUdpSocket{ nullptr };
		RTC::TransportTuple* rtpTuple{ nullptr };
		RTC::TransportTuple* rtcpTuple{ nullptr };
		// Others.
		struct sockaddr_storage rtpRemoteAddrStorage;
		struct sockaddr_storage rtcpRemoteAddrStorage;
	};
} // namespace RTC

#endif
