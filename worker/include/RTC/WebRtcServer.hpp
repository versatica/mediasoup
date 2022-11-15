#ifndef MS_RTC_WEBRTC_SERVER_HPP
#define MS_RTC_WEBRTC_SERVER_HPP

#include "Channel/ChannelRequest.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/Shared.hpp"
#include "RTC/StunPacket.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include "RTC/WebRtcTransport.hpp"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace RTC
{
	class WebRtcServer : public RTC::UdpSocket::Listener,
	                     public RTC::TcpServer::Listener,
	                     public RTC::TcpConnection::Listener,
	                     public RTC::WebRtcTransport::WebRtcTransportListener,
	                     public Channel::ChannelSocket::RequestHandler
	{
	private:
		struct ListenInfo
		{
			RTC::TransportTuple::Protocol protocol;
			std::string ip;
			std::string announcedIp;
			uint16_t port;
		};

	private:
		struct UdpSocketOrTcpServer
		{
			// Expose a constructor to use vector.emplace_back().
			UdpSocketOrTcpServer(RTC::UdpSocket* udpSocket, RTC::TcpServer* tcpServer, std::string& announcedIp)
			  : udpSocket(udpSocket), tcpServer(tcpServer), announcedIp(announcedIp)
			{
			}

			RTC::UdpSocket* udpSocket;
			RTC::TcpServer* tcpServer;
			std::string announcedIp;
		};

	public:
		WebRtcServer(RTC::Shared* shared, const std::string& id, json& data);
		~WebRtcServer();

	public:
		void FillJson(json& jsonObject) const;
		std::vector<RTC::IceCandidate> GetIceCandidates(
		  bool enableUdp, bool enableTcp, bool preferUdp, bool preferTcp);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	private:
		std::string GetLocalIceUsernameFragmentFromReceivedStunPacket(RTC::StunPacket* packet) const;
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnNonStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::WebRtcTransport::WebRtcTransportListener. */
	public:
		void OnWebRtcTransportCreated(RTC::WebRtcTransport* webRtcTransport) override;
		void OnWebRtcTransportClosed(RTC::WebRtcTransport* webRtcTransport) override;
		void OnWebRtcTransportLocalIceUsernameFragmentAdded(
		  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment) override;
		void OnWebRtcTransportLocalIceUsernameFragmentRemoved(
		  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment) override;
		void OnWebRtcTransportTransportTupleAdded(
		  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple) override;
		void OnWebRtcTransportTransportTupleRemoved(
		  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple) override;

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

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::Shared* shared{ nullptr };
		// Vector of UdpSockets and TcpServers in the user given order.
		std::vector<UdpSocketOrTcpServer> udpSocketOrTcpServers;
		// Set of WebRtcTransports.
		absl::flat_hash_set<RTC::WebRtcTransport*> webRtcTransports;
		// Map of WebRtcTransports indexed by local ICE usernameFragment.
		absl::flat_hash_map<std::string, RTC::WebRtcTransport*> mapLocalIceUsernameFragmentWebRtcTransport;
		// Map of WebRtcTransports indexed by TransportTuple.hash.
		absl::flat_hash_map<uint64_t, RTC::WebRtcTransport*> mapTupleWebRtcTransport;
	};
} // namespace RTC

#endif
