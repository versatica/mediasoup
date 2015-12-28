#define MS_CLASS "RTC::IceTransport"

#include "RTC/IceTransport.h"
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

	IceTransport::IceTransport(Listener* listener, Json::Value& data) :
		listener(listener)
	{
		MS_TRACE();

		// Check options.

		static const Json::StaticString udp("udp");
		static const Json::StaticString tcp("tcp");

		bool option_udp = true;
		bool option_tcp = true;

		if (data[udp].isBool())
			option_udp = data[udp].asBool();

		if (data[tcp].isBool())
			option_tcp = data[tcp].asBool();

		MS_DEBUG("[udp:%s, tcp:%s]",
			option_udp ? "true" : "false", option_tcp ? "true" : "false");

		// Create an ICEServer.
		char _username_fragment[16];
		char _password[16];

		std::string usernameFragment = std::string(Utils::Crypto::GetRandomHexString(_username_fragment, 16), 16);
		std::string password = std::string(Utils::Crypto::GetRandomHexString(_password, 32), 32);

		this->iceServer = new RTC::ICEServer(this, usernameFragment, password);

		// Fill IceParameters.
		this->iceLocalParameters.usernameFragment = this->iceServer->GetUsernameFragment();
		this->iceLocalParameters.password = this->iceServer->GetPassword();

		// TODO: We must check whether iceComponent is RTP or RTCP

		// Open a IPv4 UDP socket.
		if (option_udp && Settings::configuration.hasIPv4)
		{
			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket, ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 UDP socket: %s", error.what());
			}
		}

		// Open a IPv6 UDP socket.
		if (option_udp && Settings::configuration.hasIPv6)
		{
			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET6);
				RTC::IceCandidate iceCandidate(udpSocket, ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 UDP socket: %s", error.what());
			}
		}

		// Open a IPv4 TCP server.
		if (option_tcp && Settings::configuration.hasIPv4)
		{
			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET);
				RTC::IceCandidate iceCandidate(tcpServer, ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 TCP server: %s", error.what());
			}
		}

		// Open a IPv6 TCP server.
		if (option_tcp && Settings::configuration.hasIPv6)
		{
			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET6);
				RTC::IceCandidate iceCandidate(tcpServer, ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 TCP server: %s", error.what());
			}
		}
	}

	IceTransport::~IceTransport()
	{
		MS_TRACE();
	}

	void IceTransport::Close()
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

		delete this;
	}

	Json::Value IceTransport::toJson()
	{
		MS_TRACE();


		static const Json::StaticString iceLocalParameters("iceLocalParameters");
		static const Json::StaticString iceLocalCandidates("iceLocalCandidates");

		Json::Value data;

		if (this->iceComponent == IceTransport::IceComponent::RTP)
			data["iceComponent"] = "RTP";
		else
			data["iceComponent"] = "RTCP";

		data[iceLocalParameters]["usernameFragment"] = this->iceLocalParameters.usernameFragment;
		data[iceLocalParameters]["password"] = this->iceLocalParameters.password;

		data[iceLocalCandidates] = Json::arrayValue;

		for (auto iceCandidate : this->iceLocalCandidates)
		{
			data[iceLocalCandidates].append(iceCandidate.toJson());
		}

		return data;
	}

	void IceTransport::onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		// TODO
	}

	void IceTransport::onICEValidPair(RTC::ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void IceTransport::onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void IceTransport::onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void IceTransport::onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void IceTransport::onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	void IceTransport::onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onSTUNDataRecv(&source, data, len);
	}

	void IceTransport::onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onDTLSDataRecv(&source, data, len);
	}

	void IceTransport::onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTPDataRecv(&source, data, len);
	}

	void IceTransport::onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTCPDataRecv(&source, data, len);
	}

	void IceTransport::onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		// RTC::TransportSource source(connection);

		// if (RemoveSource(&source))
		// {
		// 	MS_DEBUG("valid source %s:", is_closed_by_peer ? "closed by peer" : "locally closed");
		// 	source.Dump();
		// }
	}

	void IceTransport::onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onSTUNDataRecv(&source, data, len);
	}

	void IceTransport::onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onDTLSDataRecv(&source, data, len);
	}

	void IceTransport::onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTPDataRecv(&source, data, len);
	}

	void IceTransport::onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTCPDataRecv(&source, data, len);
	}
}
