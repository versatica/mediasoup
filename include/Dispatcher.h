#ifndef MS_DISPATCHER_H
#define	MS_DISPATCHER_H

#include "common.h"
#include "handles/SignalsHandler.h"
#include "ControlProtocol/TCPServer.h"
#include "ControlProtocol/TCPConnection.h"
#include "ControlProtocol/UnixStreamSocket.h"
#include "ControlProtocol/Message.h"
#include <vector>

class Dispatcher :
	public SignalsHandler::Listener,
	public ControlProtocol::TCPServer::Listener,
	public ControlProtocol::TCPConnection::Reader,
	public ControlProtocol::UnixStreamSocket::Listener
{
public:
	Dispatcher();
	virtual ~Dispatcher();

	void Close();

/* Methods inherited from SignalsHandler::Listener. */
public:
	virtual void onSignal(SignalsHandler* signalsHandler, int signum) override;
	virtual void onSignalsHandlerClosed(SignalsHandler* signalsHandler) override;

/* Methods inherited from ControlProtocol::TCPServer::Listener. */
public:
	virtual void onControlProtocolNewTCPConnection(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection) override;
	virtual void onControlProtocolTCPConnectionClosed(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection, bool is_closed_by_peer) override;

/* Methods inherited from ControlProtocol::TCPConnection::Reader. */
public:
	virtual void onControlProtocolMessage(ControlProtocol::TCPConnection* connection, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len) override;

/* Methods inherited from ControlProtocol::UnixStreamSocket::Listener. */
public:
	virtual void onControlProtocolMessage(ControlProtocol::UnixStreamSocket* unixSocket, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len) override;
	virtual void onControlProtocolUnixStreamSocketClosed(ControlProtocol::UnixStreamSocket* unixSocket, bool is_closed_by_peer) override;

private:
	// Allocated by this:
	SignalsHandler* signalsHandler = nullptr;
	ControlProtocol::TCPServer* controlProtocolTCPServer = nullptr;
	typedef std::vector<ControlProtocol::UnixStreamSocket*> ControlProtocolUnixStreamSockets;
	ControlProtocolUnixStreamSockets controlProtocolUnixStreamSockets;
	// Others:
	bool closed = false;
};

#endif
