#ifndef MS_RTC_PLAIN_RTP_TRANSPORT_HPP
#define MS_RTC_PLAIN_RTP_TRANSPORT_HPP

#include "common.hpp"
#include "RTC/Transport.hpp"
#include <json/json.h>
#include <string>

namespace RTC
{
	class PlainRtpTransport : public RTC::Transport
	{
	public:
		struct Options
		{
			std::string remoteIp;
			uint16_t remotePort;
			std::string localIp;
			bool preferIPv4;
			bool preferIPv6;
		};

	public:
		PlainRtpTransport(uint32_t id, RTC::Transport::Listener* listener, Options& options);

	private:
		~PlainRtpTransport();

	public:
		Json::Value ToJson() const override;
		Json::Value GetStats() const override;
		void SetRemoteParameters(const std::string& ip, uint16_t port);
		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;

	private:
		void CreateSocket(const std::string& localIp);
		bool IsConnected() const override;
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
		RTC::UdpSocket* udpSocket{ nullptr };
		RTC::TransportTuple* tuple{ nullptr };
		// Others.
		struct sockaddr_storage remoteAddrStorage;
	};
} // namespace RTC

#endif
