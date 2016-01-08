#ifndef MS_RTC_TRANSPORT_H
#define MS_RTC_TRANSPORT_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/TCPConnection.h"
#include "RTC/IceCandidate.h"
#include "RTC/IceServer.h"
#include "RTC/STUNMessage.h"
#include "RTC/TransportTuple.h"
#include "RTC/DTLSTransport.h"
#include "Channel/Notifier.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class Transport :
		public RTC::UDPSocket::Listener,
		public RTC::TCPServer::Listener,
		public RTC::TCPConnection::Reader,
		public RTC::IceServer::Listener,
		public RTC::DTLSTransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onTransportClosed(RTC::Transport* transport) = 0;
		};

	private:
		static size_t maxTuples;

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, unsigned int transportId, Json::Value& data, Transport* rtpTransport = nullptr);
		virtual ~Transport();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		std::string& GetIceUsernameFragment();
		std::string& GetIcePassword();
		Transport* CreateAssociatedTransport(unsigned int transportId);

	private:
		void ClosePorts();
		bool IsAlive();
		void MayRunDTLSTransport();

	/* Private methods to unify UDP and TCP behavior. */
	private:
		void onSTUNDataRecv(RTC::TransportTuple* tuple, const MS_BYTE* data, size_t len);
		void onDTLSDataRecv(RTC::TransportTuple* tuple, const MS_BYTE* data, size_t len);
		void onRTPDataRecv(RTC::TransportTuple* tuple, const MS_BYTE* data, size_t len);
		void onRTCPDataRecv(RTC::TransportTuple* tuple, const MS_BYTE* data, size_t len);

	/* Pure virtual methods inherited from RTC::UDPSocket::Listener. */
	public:
		virtual void onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;
		virtual void onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr) override;

	/* Pure virtual methods inherited from RTC::TCPServer::Listener. */
	public:
		virtual void onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer) override;

	/* Pure virtual methods inherited from RTC::TCPConnection::Listener. */
	public:
		virtual void onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;
		virtual void onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len) override;

	/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingSTUNMessage(RTC::IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportTuple* tuple) override;
		virtual void onICESelectedTuple(IceServer* iceServer, RTC::TransportTuple* tuple) override;
		virtual void onICEConnected(IceServer* iceServer) override;
		virtual void onICECompleted(IceServer* iceServer) override;

	/* Pure virtual methods inherited from RTC::DTLSTransport::Listener. */
	public:
		virtual void onDTLSConnecting(DTLSTransport* dtlsTransport) override;
		virtual void onDTLSConnected(DTLSTransport* dtlsTransport, RTC::SRTPSession::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len) override;
		virtual void onDTLSFailed(DTLSTransport* dtlsTransport) override;
		virtual void onDTLSClosed(DTLSTransport* dtlsTransport) override;
		virtual void onOutgoingDTLSData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) override;
		virtual void onDTLSApplicationData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) override;

	public:
		// Passed by argument.
		unsigned int transportId;

	protected:
		// Others.
		bool hasIPv4udp = false;
		bool hasIPv6udp = false;
		bool hasIPv4tcp = false;
		bool hasIPv6tcp = false;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Allocated by this.
		RTC::IceServer* iceServer = nullptr;
		std::vector<RTC::UDPSocket*> udpSockets;
		std::vector<RTC::TCPServer*> tcpServers;
		RTC::DTLSTransport* dtlsTransport = nullptr;
		// Others.
		bool allocated = false;
		// Others (ICE).
		std::vector<IceCandidate> iceLocalCandidates;
		RTC::TransportTuple* selectedTuple = nullptr;
		// Others (DTLS).
		bool remoteDtlsParametersGiven = false;
		RTC::DTLSTransport::Role dtlsLocalRole = RTC::DTLSTransport::Role::AUTO;
	};

	/* Inline methods. */

	inline
	bool Transport::IsAlive()
	{
		if (this->dtlsTransport->GetState() == DTLSTransport::DtlsState::FAILED ||
		    this->dtlsTransport->GetState() == DTLSTransport::DtlsState::CLOSED)
		{
			return false;
		}

		return true;
	}

	inline
	std::string& Transport::GetIceUsernameFragment()
	{
		return this->iceServer->GetUsernameFragment();
	}

	inline
	std::string& Transport::GetIcePassword()
	{
		return this->iceServer->GetPassword();
	}
}

#endif
