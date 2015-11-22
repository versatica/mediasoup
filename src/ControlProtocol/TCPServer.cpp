#define MS_CLASS "ControlProtocol::TCPServer"

#include "ControlProtocol/TCPServer.h"
#include "Logger.h"


namespace ControlProtocol
{
	/* Instance methods. */

	TCPServer::TCPServer(Listener* listener, ControlProtocol::TCPConnection::Reader* reader, const std::string &ip, MS_PORT port) :
		::TCPServer::TCPServer(ip, port, 2048),
		listener(listener),
		reader(reader)
	{
		MS_TRACE();
	}

	void TCPServer::userOnTCPConnectionAlloc(::TCPConnection** connection)
	{
		MS_TRACE();

		// Allocate a new ControlProtocol::TCPConnection and pass it our reader.
		*connection = new ControlProtocol::TCPConnection(this->reader, 65536);
	}

	void TCPServer::userOnNewTCPConnection(::TCPConnection* connection)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->onControlProtocolNewTCPConnection(this, static_cast<ControlProtocol::TCPConnection*>(connection));
	}

	void TCPServer::userOnTCPConnectionClosed(::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->onControlProtocolTCPConnectionClosed(this, static_cast<ControlProtocol::TCPConnection*>(connection), is_closed_by_peer);
	}

	void TCPServer::userOnTCPServerClosed()
	{
		MS_TRACE();
	}
}  // namespace ControlProtocol
