#ifndef MS_CONTROL_PROTOCOL_TCP_SERVER_H
#define MS_CONTROL_PROTOCOL_TCP_SERVER_H

#include "common.h"
#include "handles/TCPServer.h"
#include "handles/TCPConnection.h"
#include "ControlProtocol/TCPConnection.h"
#include "ControlProtocol/Message.h"
#include <string>

namespace ControlProtocol
{
	class TCPServer : public ::TCPServer
	{
	public:
		class Listener
		{
		public:
			virtual void onControlProtocolNewTCPConnection(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection) = 0;
			virtual void onControlProtocolTCPConnectionClosed(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection, bool is_closed_by_peer) = 0;
		};

	public:
		TCPServer(Listener* listener, ControlProtocol::TCPConnection::Reader* reader, const std::string &ip, MS_PORT port);

	/* Pure virtual methods inherited from ::TCPServer. */
	public:
		virtual void userOnTCPConnectionAlloc(::TCPConnection** connection) override;
		virtual void userOnNewTCPConnection(::TCPConnection* connection) override;
		virtual void userOnTCPConnectionClosed(::TCPConnection* connection, bool is_closed_by_peer) override;
		virtual void userOnTCPServerClosed() override;

	private:
		// Passed by argument:
		Listener* listener = nullptr;
		ControlProtocol::TCPConnection::Reader* reader = nullptr;
	};
}  // namespace ControlProtocol

#endif
