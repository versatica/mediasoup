#define MS_CLASS "RTC::TcpServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TcpServer.hpp"
#include "Logger.hpp"
#include "RTC/PortManager.hpp"
#include <string>

namespace RTC
{
	/* Static. */

	static constexpr size_t MaxTcpConnectionsPerServer{ 10 };

	/* Instance methods. */

	TcpServer::TcpServer(Listener* listener, RTC::TcpConnection::Listener* connListener, std::string& ip)
	  : // This may throw.
	    ::TcpServerHandler::TcpServerHandler(RTC::PortManager::BindTcp(ip), 256), listener(listener),
	    connListener(connListener), fixedPort(false)
	{
		MS_TRACE();
	}

	TcpServer::TcpServer(
	  Listener* listener, RTC::TcpConnection::Listener* connListener, std::string& ip, uint16_t port)
	  : // This may throw.
	    ::TcpServerHandler::TcpServerHandler(RTC::PortManager::BindTcp(ip, port), 256),
	    listener(listener), connListener(connListener), fixedPort(true)
	{
		MS_TRACE();
	}

	TcpServer::~TcpServer()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			RTC::PortManager::UnbindTcp(this->localIp, this->localPort);
		}
	}

	void TcpServer::UserOnTcpConnectionAlloc()
	{
		MS_TRACE();

		// Allow just MaxTcpConnectionsPerServer.
		if (GetNumConnections() >= MaxTcpConnectionsPerServer)
		{
			MS_ERROR("cannot handle more than %zu connections", MaxTcpConnectionsPerServer);

			return;
		}

		// Allocate a new RTC::TcpConnection for the TcpServer to handle it.
		auto* connection = new RTC::TcpConnection(this->connListener, 65536);

		// Accept it.
		AcceptTcpConnection(connection);
	}

	void TcpServer::UserOnTcpConnectionClosed(::TcpConnectionHandler* connection)
	{
		MS_TRACE();

		this->listener->OnRtcTcpConnectionClosed(this, static_cast<RTC::TcpConnection*>(connection));
	}
} // namespace RTC
