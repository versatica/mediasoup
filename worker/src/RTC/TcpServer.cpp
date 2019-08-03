#define MS_CLASS "RTC::TcpServer"
// #define MS_LOG_DEV

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
	    ::TcpServer::TcpServer(PortManager::BindTcp(ip), 256), listener(listener),
	    connListener(connListener)
	{
		MS_TRACE();
	}

	TcpServer::~TcpServer()
	{
		MS_TRACE();

		PortManager::UnbindTcp(this->localIp, this->localPort);
	}

	void TcpServer::UserOnTcpConnectionAlloc(::TcpConnection** connection)
	{
		MS_TRACE();

		// Allocate a new RTC::TcpConnection for the TcpServer to handle it.
		*connection = new RTC::TcpConnection(this->connListener, 65536);
	}

	bool TcpServer::UserOnNewTcpConnection(::TcpConnection* connection)
	{
		MS_TRACE();

		if (GetNumConnections() >= MaxTcpConnectionsPerServer)
		{
			MS_ERROR("cannot handle more than %zu connections", MaxTcpConnectionsPerServer);

			return false;
		}

		return true;
	}

	void TcpServer::UserOnTcpConnectionClosed(::TcpConnection* connection)
	{
		MS_TRACE();

		this->listener->OnRtcTcpConnectionClosed(this, static_cast<RTC::TcpConnection*>(connection));
	}
} // namespace RTC
