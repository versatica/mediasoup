#define MS_CLASS "RTC::TCPServer"

#include "RTC/TCPServer.h"
#include "RTC/STUNMessage.h"
#include "RTC/RTPPacket.h"
#include "Settings.h"
#include "Utils.h"
#include "DepLibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>

#define MAX_BIND_ATTEMPTS 20
#define MAX_TCP_CONNECTIONS_PER_SERVER 10

/* Static methods for UV callbacks. */

static inline
void on_error_close(uv_handle_t* handle)
{
	delete handle;
}

namespace RTC
{
	/* Class variables. */

	struct sockaddr_storage TCPServer::sockaddrStorageIPv4;
	struct sockaddr_storage TCPServer::sockaddrStorageIPv6;
	uint16_t TCPServer::minPort;
	uint16_t TCPServer::maxPort;
	std::unordered_map<uint16_t, bool> TCPServer::availableIPv4Ports;
	std::unordered_map<uint16_t, bool> TCPServer::availableIPv6Ports;

	/* Class methods. */

	void TCPServer::ClassInit()
	{
		MS_TRACE();

		int err;

		if (!Settings::configuration.rtcListenIPv4.empty())
		{
			err = uv_ip4_addr(Settings::configuration.rtcListenIPv4.c_str(), 0, (struct sockaddr_in*)&RTC::TCPServer::sockaddrStorageIPv4);
			if (err)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));
		}

		if (!Settings::configuration.rtcListenIPv6.empty())
		{
			err = uv_ip6_addr(Settings::configuration.rtcListenIPv6.c_str(), 0, (struct sockaddr_in6*)&RTC::TCPServer::sockaddrStorageIPv6);
			if (err)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
		}

		TCPServer::minPort = Settings::configuration.rtcMinPort;
		TCPServer::maxPort = Settings::configuration.rtcMaxPort;

		uint16_t i = RTC::TCPServer::minPort;
		do
		{
			RTC::TCPServer::availableIPv4Ports[i] = true;
			RTC::TCPServer::availableIPv6Ports[i] = true;
		}
		while (i++ != RTC::TCPServer::maxPort);
	}

	uv_tcp_t* TCPServer::GetRandomPort(int address_family)
	{
		MS_TRACE();

		if (address_family == AF_INET && !Settings::configuration.hasIPv4)
			MS_THROW_ERROR("IPv4 family not available for RTC");
		else if (address_family == AF_INET6 && !Settings::configuration.hasIPv6)
			MS_THROW_ERROR("IPv6 family not available for RTC");

		int err;
		uv_tcp_t* uvHandle = nullptr;
		struct sockaddr_storage bind_addr;
		const char* listen_ip;
		uint16_t initial_port;
		uint16_t iterating_port;
		uint16_t attempt = 0;
		uint16_t bind_attempt = 0;
		int flags = 0;
		std::unordered_map<uint16_t, bool>* available_ports;

		switch (address_family)
		{
			case AF_INET:
				available_ports = &RTC::TCPServer::availableIPv4Ports;
				bind_addr = RTC::TCPServer::sockaddrStorageIPv4;
				listen_ip = Settings::configuration.rtcListenIPv4.c_str();
				break;

			case AF_INET6:
				available_ports = &RTC::TCPServer::availableIPv6Ports;
				bind_addr = RTC::TCPServer::sockaddrStorageIPv6;
				listen_ip = Settings::configuration.rtcListenIPv6.c_str();
				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_TCP_IPV6ONLY;
				break;

			default:
				MS_THROW_ERROR("invalid address family given");
				break;
		}

		// Choose a random first port to start from.
		initial_port = (uint16_t)Utils::Crypto::GetRandomUInt((uint32_t)RTC::TCPServer::minPort, (uint32_t)RTC::TCPServer::maxPort);

		iterating_port = initial_port;

		// Iterate the RTC TCP ports until getting one available.
		// Fail also after bind() fails N times in theorically available ports.
		while (true)
		{
			attempt++;

			// Increase the iterate port) within the range of RTC TCP ports.
			if (iterating_port < RTC::TCPServer::maxPort)
				iterating_port += 1;
			else
				iterating_port = RTC::TCPServer::minPort;

			// Check whether the chosen port is available.
			if (!(*available_ports)[iterating_port])
			{
				MS_DEBUG("port in use, trying again [port:%" PRIu16 ", attempt:%" PRIu16 "]", iterating_port, attempt);

				// If we have tried all the ports in the range raise an error.
				if (iterating_port == initial_port)
				{
					MS_THROW_ERROR("no more available ports for IP '%s'", listen_ip);
				}

				continue;
			}

			// Here we already have a theorically available port.
			// Now let's check whether no other process is listening into it.

			// Set the chosen port into the sockaddr struct(s).
			switch (address_family)
			{
				case AF_INET:
					((struct sockaddr_in*)&bind_addr)->sin_port = htons(iterating_port);
					break;
				case AF_INET6:
					((struct sockaddr_in6*)&bind_addr)->sin6_port = htons(iterating_port);
					break;
			}

			// Try to bind on it.
			bind_attempt++;

			uvHandle = new uv_tcp_t();

			err = uv_tcp_init(DepLibUV::GetLoop(), uvHandle);
			if (err)
			{
				delete uvHandle;
				MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
			}

			err = uv_tcp_bind(uvHandle, (const struct sockaddr*)&bind_addr, flags);
			if (err)
			{
				MS_WARN("uv_tcp_bind() failed [port:%" PRIu16 ", attempt:%" PRIu16 "]: %s", attempt, iterating_port, uv_strerror(err));

				uv_close((uv_handle_t*)uvHandle, (uv_close_cb)on_error_close);

				// If bind() fails due to "too many open files" stop here.
				if (err == UV_EMFILE)
				{
					MS_THROW_ERROR("uv_tcp_bind() fails due to many open files");
				}

				// If bind() fails for more that MAX_BIND_ATTEMPTS then raise an error.
				if (bind_attempt > MAX_BIND_ATTEMPTS)
				{
					MS_THROW_ERROR("uv_tcp_bind() fails more than %u times for IP '%s'", MAX_BIND_ATTEMPTS, listen_ip);
				}

				// If we have tried all the ports in the range raise an error.
				if (iterating_port == initial_port)
				{
					MS_THROW_ERROR("no more available ports for IP '%s'", listen_ip);
				}

				continue;
			}

			// Set the port as unavailable.
			(*available_ports)[iterating_port] = false;

			MS_DEBUG("bind success [ip:%s, port:%" PRIu16 ", attempt:%" PRIu16 "]",
					listen_ip, iterating_port, attempt);

			return uvHandle;
		};
	}

	/* Instance methods. */

	TCPServer::TCPServer(Listener* listener, RTC::TCPConnection::Listener* connListener, int address_family) :
		// Provide the parent class constructor with a UDP uv handle.
		// NOTE: This may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		::TCPServer::TCPServer(GetRandomPort(address_family), 256),
		listener(listener),
		connListener(connListener)
	{
		MS_TRACE();
	}

	void TCPServer::userOnTCPConnectionAlloc(::TCPConnection** connection)
	{
		MS_TRACE();

		// Allocate a new RTC::TCPConnection for the TCPServer to handle it.
		*connection = new RTC::TCPConnection(this->connListener, 65536);
	}

	void TCPServer::userOnNewTCPConnection(::TCPConnection* connection)
	{
		MS_TRACE();

		// Allow just MAX_TCP_CONNECTIONS_PER_SERVER.
		if (GetNumConnections() > MAX_TCP_CONNECTIONS_PER_SERVER)
			connection->Close();
	}

	void TCPServer::userOnTCPConnectionClosed(::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		// NOTE: Don't do it if closing (since at this point the listener is already freed).
		// At the end, this is just called if the connection was remotely closed.
		if (!IsClosing())
			this->listener->onRTCTCPConnectionClosed(this, static_cast<RTC::TCPConnection*>(connection), is_closed_by_peer);
	}

	void TCPServer::userOnTCPServerClosed()
	{
		MS_TRACE();

		// Mark the port as available again.
		if (this->localAddr.ss_family == AF_INET)
			RTC::TCPServer::availableIPv4Ports[this->localPort] = true;
		else if (this->localAddr.ss_family == AF_INET6)
			RTC::TCPServer::availableIPv6Ports[this->localPort] = true;
	}
}
