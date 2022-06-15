#ifndef MS_RTC_WEBRTC_SERVER_HPP
#define MS_RTC_WEBRTC_SERVER_HPP

#include "Channel/ChannelRequest.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/StunPacket.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace RTC
{
	class WebRtcServer : public RTC::UdpSocket::Listener,
	                     public RTC::TcpServer::Listener,
	                     public RTC::TcpConnection::Listener,
	                     public RTC::IceServer::Listener
	{
	private:
		struct ListenIpPort
		{
			std::string ip;
			std::string announcedIp;
			uint16_t port;
		};

	public:
		WebRtcServer(const std::string& id, json& data);
		~WebRtcServer();

	public:
		void FillJson(json& jsonObject) const;
		void HandleRequest(Channel::ChannelRequest* request);

	private:
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnNonStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

		/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		void OnRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection) override;

		/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		void OnTcpConnectionPacketReceived(
		  RTC::TcpConnection* connection, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		void OnIceServerSendStunPacket(
		  const RTC::IceServer* iceServer,
		  const RTC::StunPacket* packet,
		  RTC::TransportTuple* tuple) override;
		void OnIceServerSelectedTuple(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		void OnIceServerConnected(const RTC::IceServer* iceServer) override;
		void OnIceServerCompleted(const RTC::IceServer* iceServer) override;
		void OnIceServerDisconnected(const RTC::IceServer* iceServer) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Map of UdpSocket/TcpServer and local announced IP (if any).
		absl::flat_hash_map<RTC::UdpSocket*, std::string> udpSockets;
		absl::flat_hash_map<RTC::TcpServer*, std::string> tcpServers;
		std::vector<RTC::IceCandidate> iceCandidates;
	};
} // namespace RTC

#endif
