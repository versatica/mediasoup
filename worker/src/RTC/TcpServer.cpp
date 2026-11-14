#define MS_CLASS "RTC::TcpServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TcpServer.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t TcpConnectionBufferSize{ 65536 };

	/* Instance methods. */

	TcpServer::TcpServer(
	  Listener* listener,
	  RTC::TcpConnection::Listener* connListener,
	  SharedInterface* shared,
	  std::string& ip,
	  const std::string& announcedAddress,
	  uint16_t port,
	  const RTC::Transport::SocketFlags& flags)
	  : // This may throw.
	    TcpServerHandle::TcpServerHandle(RTC::PortManager::BindTcp(ip, port, flags)),
	    listener(listener),
	    connListener(connListener),
	    shared(shared),
	    announcedAddress(announcedAddress),
	    fixedPort(true)
	{
		MS_TRACE();
	}

	TcpServer::TcpServer(
	  Listener* listener,
	  RTC::TcpConnection::Listener* connListener,
	  SharedInterface* shared,
	  std::string& ip,
	  const std::string& announcedAddress,
	  uint16_t minPort,
	  uint16_t maxPort,
	  const RTC::Transport::SocketFlags& flags,
	  RTC::PortManager::PortRangeKey& portRangeKey)
	  : // This may throw.
	    TcpServerHandle::TcpServerHandle(
	      RTC::PortManager::BindTcp(ip, minPort, maxPort, flags, portRangeKey)),
	    listener(listener),
	    connListener(connListener),
	    shared(shared),
	    announcedAddress(announcedAddress),
	    portRangeKey(portRangeKey)
	{
		MS_TRACE();
	}

	TcpServer::~TcpServer()
	{
		MS_TRACE();

		if (!this->fixedPort)
		{
			RTC::PortManager::Unbind(this->portRangeKey, this->localPort);
		}
	}

	void TcpServer::UserOnTcpConnectionAlloc()
	{
		MS_TRACE();

		// Allocate a new RTC::TcpConnection for the TcpServer to handle it.
		auto* const connection = new RTC::TcpConnection(
		  this->connListener, this->shared, this->announcedAddress, TcpConnectionBufferSize);

		// Accept it.
		AcceptTcpConnection(connection);
	}

	void TcpServer::UserOnTcpConnectionClosed(TcpConnectionHandle* connection)
	{
		MS_TRACE();

		this->listener->OnRtcTcpConnectionClosed(this, static_cast<RTC::TcpConnection*>(connection));
	}
} // namespace RTC
