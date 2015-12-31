#define MS_CLASS "RTC::Transport"

#include "RTC/Transport.h"
#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cmath>  // std::pow()

#define ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP  (std::pow(2, 24) * 64) + (std::pow(2, 8) * (10000 + 30000)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP  (std::pow(2, 24) * 64) + (std::pow(2, 8) * (10000 + 40000)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP  (std::pow(2, 24) * 64) + (std::pow(2, 8) *  (5000 + 30000)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP  (std::pow(2, 24) * 64) + (std::pow(2, 8) *  (5000 + 40000)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTCP (std::pow(2, 24) * 64) + (std::pow(2, 8) * (10000 + 30000)) + (256 - 2)
#define ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTCP (std::pow(2, 24) * 64) + (std::pow(2, 8) * (10000 + 40000)) + (256 - 2)
#define ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTCP (std::pow(2, 24) * 64) + (std::pow(2, 8) *  (5000 + 30000)) + (256 - 2)
#define ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTCP (std::pow(2, 24) * 64) + (std::pow(2, 8) *  (5000 + 40000)) + (256 - 2)

namespace RTC
{
	/* Instance methods. */

	Transport::Transport(Listener* listener, unsigned int transportId, Json::Value& data, Transport* rtpTransport) :
		transportId(transportId),
		listener(listener)
	{
		MS_TRACE();

		static const Json::StaticString k_udp("udp");
		static const Json::StaticString k_tcp("tcp");

		bool try_IPv4_udp = false;
		bool try_IPv6_udp = false;
		bool try_IPv4_tcp = false;
		bool try_IPv6_tcp = false;

		// RTP transport.
		if (!rtpTransport)
		{
			this->iceComponent = IceComponent::RTP;

			this->iceServer = new RTC::IceServer(this,
				Utils::Crypto::GetRandomString(16),
				Utils::Crypto::GetRandomString(32));

			if (data[k_udp].isBool())
				try_IPv4_udp = try_IPv6_udp = data[k_udp].asBool();

			if (data[k_tcp].isBool())
				try_IPv4_tcp = try_IPv6_tcp = data[k_tcp].asBool();
		}
		// RTCP transport associated to a given RTP transport.
		else
		{
			this->iceComponent = IceComponent::RTCP;

			this->iceServer = new RTC::IceServer(this,
				rtpTransport->GetIceUsernameFragment(),
				rtpTransport->GetIcePassword());

			try_IPv4_udp = rtpTransport->hasIPv4udp;
			try_IPv6_udp = rtpTransport->hasIPv6udp;
			try_IPv4_tcp = rtpTransport->hasIPv4tcp;
			try_IPv6_tcp = rtpTransport->hasIPv6tcp;
		}

		// Open a IPv4 UDP socket.
		if (try_IPv4_udp && Settings::configuration.hasIPv4)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTCP;

			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv4udp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 UDP socket: %s", error.what());
			}
		}

		// Open a IPv6 UDP socket.
		if (try_IPv6_udp && Settings::configuration.hasIPv6)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTCP;

			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET6);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv6udp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 UDP socket: %s", error.what());
			}
		}

		// Open a IPv4 TCP server.
		if (try_IPv4_tcp && Settings::configuration.hasIPv4)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTCP;

			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv4tcp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 TCP server: %s", error.what());
			}
		}

		// Open a IPv6 TCP server.
		if (try_IPv6_tcp && Settings::configuration.hasIPv6)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTCP;

			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET6);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv6tcp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 TCP server: %s", error.what());
			}
		}

		// Ensure there is at least one IP:port binding.
		if (!this->udpSockets.size() && !this->tcpServers.size())
		{
			Close();

			MS_THROW_ERROR("could not open any IP:port");
		}

		// Hack to avoid that Close() above attempts to delete this.
		this->allocated = true;
	}

	Transport::~Transport()
	{
		MS_TRACE();
	}

	void Transport::Close()
	{
		MS_TRACE();

		if (this->iceServer)
			this->iceServer->Close();

		// if (this->dtlsAgent)
			// this->dtlsAgent->Close();

		// if (this->srtpRecvSession)
			// this->srtpRecvSession->Close();

		// if (this->srtpSendSession)
			// this->srtpSendSession->Close();

		for (auto socket : this->udpSockets)
			socket->Close();

		for (auto server : this->tcpServers)
			server->Close();

		if (this->allocated)
		{
			// Notify the listener.
			this->listener->onTransportClosed(this);

			delete this;
		}
	}

	Json::Value Transport::Dump()
	{
		MS_TRACE();

		return toJson();
	}

	Json::Value Transport::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_iceComponent("iceComponent");
		static const Json::StaticString k_iceLocalParameters("iceLocalParameters");
		static const Json::StaticString k_usernameFragment("usernameFragment");
		static const Json::StaticString k_password("password");
		static const Json::StaticString k_iceLocalCandidates("iceLocalCandidates");
		static const Json::StaticString v_RTP("RTP");
		static const Json::StaticString v_RTCP("RTCP");

		Json::Value data;

		if (this->iceComponent == IceComponent::RTP)
			data[k_iceComponent] = v_RTP;
		else
			data[k_iceComponent] = v_RTCP;

		data[k_iceLocalParameters][k_usernameFragment] = this->iceServer->GetUsernameFragment();
		data[k_iceLocalParameters][k_password] = this->iceServer->GetPassword();

		data[k_iceLocalCandidates] = Json::arrayValue;

		for (auto iceCandidate : this->iceLocalCandidates)
		{
			data[k_iceLocalCandidates].append(iceCandidate.toJson());
		}

		return data;
	}

	Transport* Transport::CreateAssociatedTransport(unsigned int transportId)
	{
		MS_TRACE();

		static Json::Value nullData(Json::nullValue);

		if (this->iceComponent != IceComponent::RTP)
			MS_THROW_ERROR("cannot call CreateAssociatedTransport() on a RTCP Transport");

		return new RTC::Transport(this->listener, transportId, nullData, this);
	}

	void Transport::onOutgoingSTUNMessage(RTC::IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		// TODO
	}

	void Transport::onICEValidPair(RTC::IceServer* iceServer, RTC::TransportSource* source, bool has_use_candidate)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void Transport::onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void Transport::onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void Transport::onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void Transport::onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	void Transport::onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onSTUNDataRecv(&source, data, len);
	}

	void Transport::onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onDTLSDataRecv(&source, data, len);
	}

	void Transport::onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTPDataRecv(&source, data, len);
	}

	void Transport::onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTCPDataRecv(&source, data, len);
	}

	void Transport::onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		// RTC::TransportSource source(connection);

		// if (RemoveSource(&source))
		// {
		// 	MS_DEBUG("valid source %s:", is_closed_by_peer ? "closed by peer" : "locally closed");
		// 	source.Dump();
		// }
	}

	void Transport::onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onSTUNDataRecv(&source, data, len);
	}

	void Transport::onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onDTLSDataRecv(&source, data, len);
	}

	void Transport::onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTPDataRecv(&source, data, len);
	}

	void Transport::onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTCPDataRecv(&source, data, len);
	}
}
