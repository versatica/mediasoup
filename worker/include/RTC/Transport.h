#ifndef MS_RTC_TRANSPORT_H
#define MS_RTC_TRANSPORT_H

#include "common.h"
#include "RTC/IceCandidate.h"
#include "RTC/IceServer.h"
#include "RTC/STUNMessage.h"
#include "RTC/TransportSource.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/TCPConnection.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class Transport :
		public RTC::IceServer::Listener,
		public RTC::UDPSocket::Listener,
		public RTC::TCPServer::Listener,
		public RTC::TCPConnection::Reader
	{
	public:
		class Listener
		{
		public:
			// TODO
		};

	public:
		enum class IceComponent
		{
			RTP  = 1,
			RTCP = 2
		};

	public:
		typedef struct IceParameters
		{
			std::string usernameFragment;
			std::string password;
		} IceParameters;

	public:
		Transport(Listener* listener, unsigned int transportId, Json::Value& data, Transport* rtpTransport = nullptr);
		virtual ~Transport();

		void Close();
		Json::Value toJson();
		std::string& GetIceUsernameFragment();
		std::string& GetIcePassword();
		Transport* CreateAssociatedTransport(unsigned int transportId);

	/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingSTUNMessage(RTC::IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) override;
		virtual void onICEValidPair(RTC::IceServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) override;

	/* Private methods to unify UDP and TCP behavior. */
	private:
		void onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);
		void onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len);

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

	public:
		bool hasIPv4udp = false;
		bool hasIPv6udp = false;
		bool hasIPv4tcp = false;
		bool hasIPv6tcp = false;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		unsigned int transportId;
		RTC::Transport::IceComponent iceComponent;
		// Allocated by this.
		RTC::IceServer* iceServer = nullptr;
		std::vector<RTC::UDPSocket*> udpSockets;
		std::vector<RTC::TCPServer*> tcpServers;
		// Others.
		IceParameters iceLocalParameters;
		std::vector<IceCandidate> iceLocalCandidates;
		bool allocated = false;
	};

	/* Inline methods. */

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
