#define MS_CLASS "RTC::Transport"

#include "RTC/ICETransport.h"
#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	ICETransport::ICETransport(Listener* listener, Json::Value& data) :
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

		// Open a IPv4 UDP socket.
		if (option_udp && Settings::configuration.hasIPv4)
		{
			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket);

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
				RTC::IceCandidate iceCandidate(udpSocket);

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
				RTC::IceCandidate iceCandidate(tcpServer);

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
				RTC::IceCandidate iceCandidate(tcpServer);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 TCP server: %s", error.what());
			}
		}
	}

	ICETransport::~ICETransport()
	{
		MS_TRACE();
	}

	void ICETransport::Close()
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

	Json::Value ICETransport::toJson()
	{
		MS_TRACE();


		static const Json::StaticString iceLocalParameters("iceLocalParameters");
		static const Json::StaticString iceCandidates("iceCandidates");

		Json::Value data;

		if (this->iceComponent == ICETransport::IceComponent::RTP)
			data["iceComponent"] = "RTP";
		else
			data["iceComponent"] = "RTCP";

		data[iceLocalParameters]["usernameFragment"] = this->iceLocalParameters.usernameFragment;
		data[iceLocalParameters]["password"] = this->iceLocalParameters.password;

		data[iceCandidates] = Json::arrayValue;

		for (auto iceCandidate : this->iceLocalCandidates)
		{
			data[iceCandidates].append(iceCandidate.toJson());
		}

		return data;
	}

	void ICETransport::onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		// TODO
	}

	void ICETransport::onICEValidPair(RTC::ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void ICETransport::onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void ICETransport::onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void ICETransport::onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void ICETransport::onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	void ICETransport::onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onSTUNDataRecv(&source, data, len);
	}

	void ICETransport::onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onDTLSDataRecv(&source, data, len);
	}

	void ICETransport::onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTPDataRecv(&source, data, len);
	}

	void ICETransport::onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTCPDataRecv(&source, data, len);
	}

	void ICETransport::onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		// RTC::TransportSource source(connection);

		// if (RemoveSource(&source))
		// {
		// 	MS_DEBUG("valid source %s:", is_closed_by_peer ? "closed by peer" : "locally closed");
		// 	source.Dump();
		// }
	}

	void ICETransport::onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onSTUNDataRecv(&source, data, len);
	}

	void ICETransport::onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onDTLSDataRecv(&source, data, len);
	}

	void ICETransport::onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTPDataRecv(&source, data, len);
	}

	void ICETransport::onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTCPDataRecv(&source, data, len);
	}
}
