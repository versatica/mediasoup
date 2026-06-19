#ifndef MS_RTC_WEBRTC_SERVER_HPP
#define MS_RTC_WEBRTC_SERVER_HPP

#include "Channel/ChannelRequest.hpp"
#include "RTC/ICE/IceCandidate.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include "RTC/WebRtcTransport.hpp"
#include "SharedInterface.hpp"
#include "TransportTuple.hpp"
#include <flatbuffers/flatbuffers.h>
#include <ankerl/unordered_dense.h>
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
		struct UdpSocketOrTcpServer
		{
			// Expose a constructor to use vector.emplace_back().
			UdpSocketOrTcpServer(
			  RTC::UdpSocket* udpSocket,
			  RTC::TcpServer* tcpServer,
			  std::string& announcedAddress,
			  bool exposeInternalIp)
			  : udpSocket(udpSocket),
			    tcpServer(tcpServer),
			    announcedAddress(announcedAddress),
			    exposeInternalIp(exposeInternalIp)
			{
			}

			RTC::UdpSocket* udpSocket;
			RTC::TcpServer* tcpServer;
			std::string announcedAddress;
			bool exposeInternalIp;
		};

	private:
		static std::string GetLocalIceUsernameFragmentFromReceivedStunPacket(
		  const RTC::ICE::StunPacket* packet);

	public:
		WebRtcServer(
		  SharedInterface* shared,
		  const std::string& id,
		  const flatbuffers::Vector<flatbuffers::Offset<FBS::Transport::ListenInfo>>* listenInfos);
		~WebRtcServer() override;

	public:
		flatbuffers::Offset<FBS::WebRtcServer::DumpResponse> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;
		std::vector<RTC::ICE::IceCandidate> GetIceCandidates(
		  bool enableUdp, bool enableTcp, bool preferUdp, bool preferTcp) const;

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	private:
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len, size_t bufferLen);
		void OnStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnNonStunDataReceived(
		  RTC::TransportTuple* tuple, const uint8_t* data, size_t len, size_t bufferLen);

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
		  RTC::UdpSocket* socket,
		  const uint8_t* data,
		  size_t len,
		  size_t bufferLen,
		  const struct sockaddr* remoteAddr) override;

		/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		void OnRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection) override;

		/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		void OnTcpConnectionPacketReceived(
		  RTC::TcpConnection* connection, const uint8_t* data, size_t len, size_t bufferLen) override;
		const std::string& GetId() const
		{
			return this->id;
		}

	private:
		// Passed by argument.
		std::string id;
		// Passed by argument.
		SharedInterface* shared{ nullptr };
		// Vector of UdpSockets and TcpServers in the user given order.
		std::vector<UdpSocketOrTcpServer> udpSocketOrTcpServers;
		// Set of WebRtcTransports.
		ankerl::unordered_dense::set<RTC::WebRtcTransport*> webRtcTransports;
		// Map of WebRtcTransports indexed by local ICE usernameFragment.
		ankerl::unordered_dense::map<std::string, RTC::WebRtcTransport*> mapLocalIceUsernameFragmentWebRtcTransport;
		// Map of WebRtcTransports indexed by TransportTuple::TupleKey.
		ankerl::unordered_dense::
		  map<RTC::TransportTuple::TupleKey, RTC::WebRtcTransport*, RTC::TransportTuple::TupleKeyHash>
		    mapTupleWebRtcTransport;
		// Whether the destructor has been called.
		bool closing{ false };
	};
} // namespace RTC

#endif
