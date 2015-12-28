#ifndef MS_RTC_ICE_TRANSPORT_H
#define MS_RTC_ICE_TRANSPORT_H

#include "common.h"
#include "RTC/IceCandidate.h"
#include "RTC/ICEServer.h"
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
	class ICETransport :
		public RTC::ICEServer::Listener,
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
		ICETransport(Listener* listener, Json::Value& data);
		virtual ~ICETransport();

		void Close();
		Json::Value toJson();
		IceComponent GetIceComponent();  // TODO: needed?
		IceParameters* GetIceLocalParameters();    // TODO: needed?
		std::vector<IceCandidate>& GetIceLocalCandidates();
		// ICETransport* CreateAssociatedGatherer();  // TODO: let's see.

	/* Pure virtual methods inherited from RTC::ICEServer::Listener. */
	public:
		virtual void onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source) override;
		virtual void onICEValidPair(RTC::ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate) override;

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

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Allocated by this.
		RTC::ICEServer* iceServer = nullptr;
		std::vector<RTC::UDPSocket*> udpSockets;
		std::vector<RTC::TCPServer*> tcpServers;
		// Others.
		IceComponent iceComponent = IceComponent::RTP;
		IceParameters iceLocalParameters;
		std::vector<IceCandidate> iceLocalCandidates;
	};

	/* Inline methods. */

	inline
	ICETransport::IceParameters* ICETransport::GetIceLocalParameters()
	{
		return &(this->iceLocalParameters);
	}
}

#endif
