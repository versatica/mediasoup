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
	MS_PORT TCPServer::minPort;
	MS_PORT TCPServer::maxPort;
	TCPServer::AvailablePorts TCPServer::availableIPv4Ports;
	TCPServer::AvailablePorts TCPServer::availableIPv6Ports;

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

		MS_PORT i = RTC::TCPServer::minPort;
		do
		{
			RTC::TCPServer::availableIPv4Ports[i] = true;
			RTC::TCPServer::availableIPv6Ports[i] = true;
		}
		while (i++ != RTC::TCPServer::maxPort);
	}

	RTC::TCPServer* TCPServer::Factory(Listener* listener, RTC::TCPConnection::Reader* reader, int address_family)
	{
		MS_TRACE();

		uv_tcp_t* uvHandles[1];

		// NOTE: This call may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		RandomizePort(address_family, uvHandles, false);

		return new RTC::TCPServer(listener, reader, uvHandles[0]);
	}

	void TCPServer::PairFactory(Listener* listener, RTC::TCPConnection::Reader* reader, int address_family, RTC::TCPServer* servers[])
	{
		MS_TRACE();

		uv_tcp_t* uvHandles[2];

		// NOTE: This call may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		RandomizePort(address_family, uvHandles, true);

		servers[0] = new RTC::TCPServer(listener, reader, uvHandles[0]);
		servers[1] = new RTC::TCPServer(listener, reader, uvHandles[1]);
	}

	void TCPServer::RandomizePort(int address_family, uv_tcp_t* uvHandles[], bool pair)
	{
		MS_TRACE();

		if (address_family == AF_INET && !Settings::configuration.hasIPv4)
			MS_THROW_ERROR("IPv4 family not available for RTC");
		else if (address_family == AF_INET6 && !Settings::configuration.hasIPv6)
			MS_THROW_ERROR("IPv6 family not available for RTC");

		int err;
		struct sockaddr_storage first_bind_addr;
		struct sockaddr_storage second_bind_addr;
		const char* listenIP;
		MS_PORT random_first_port;
		MS_PORT iterate_first_port;
		MS_PORT iterate_second_port;
		uint16_t attempt = 0;
		uint16_t bindAttempt = 0;
		int flags = 0;
		AvailablePorts* available_ports;

		switch (address_family)
		{
			case AF_INET:
				available_ports = &RTC::TCPServer::availableIPv4Ports;
				first_bind_addr = RTC::TCPServer::sockaddrStorageIPv4;
				second_bind_addr = RTC::TCPServer::sockaddrStorageIPv4;
				listenIP = Settings::configuration.rtcListenIPv4.c_str();
				break;

			case AF_INET6:
				available_ports = &RTC::TCPServer::availableIPv6Ports;
				first_bind_addr = RTC::TCPServer::sockaddrStorageIPv6;
				second_bind_addr = RTC::TCPServer::sockaddrStorageIPv6;
				listenIP = Settings::configuration.rtcListenIPv6.c_str();
				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_TCP_IPV6ONLY;
				break;

			default:
				MS_THROW_ERROR("invalid address family given");
				break;
		}

		// Choose a random first port to start from.
		random_first_port = (MS_PORT)Utils::Crypto::GetRandomUInt((unsigned int)RTC::TCPServer::minPort, (unsigned int)RTC::TCPServer::maxPort);
		// Make it even if pair is requested.
		if (pair)
			random_first_port &= ~1;

		// Set iterate_xxxx_port.
		iterate_first_port = random_first_port;

		// Iterate the RTC TCP ports until get one (or two) available (or fail if not).
		// Fail also after bind() fails N times in theorically available RTC TCP ports.
		while (true)
		{
			attempt++;

			// Increase the iterate port(s) within the range of RTC TCP ports.
			if (!pair)
			{
				if (iterate_first_port < RTC::TCPServer::maxPort)
					iterate_first_port += 1;
				else
					iterate_first_port = RTC::TCPServer::minPort;
			}
			else
			{
				if (iterate_first_port < RTC::TCPServer::maxPort - 1)
					iterate_first_port += 2;
				else
					iterate_first_port = RTC::TCPServer::minPort;
				iterate_second_port = iterate_first_port + 1;
			}

			if (!pair)
				MS_DEBUG("attempt: %u | trying port %u" , (unsigned int)attempt, (unsigned int)iterate_first_port);
			else
				MS_DEBUG("attempt: %u | trying ports %u and %u" , (unsigned int)attempt, (unsigned int)iterate_first_port, (unsigned int)iterate_second_port);

			// Check whether the chosen port is available.
			if (!(*available_ports)[iterate_first_port])
			{
				MS_DEBUG("attempt: %u | port %u is in use, try again", (unsigned int)attempt, (unsigned int)iterate_first_port);

				// If we have tried all the ports in the range raise an error.
				if (iterate_first_port == random_first_port)
				{
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIP);
				}

				continue;
			}

			// Check also the odd port (if requested).
			if (pair && !(*available_ports)[iterate_second_port])
			{
				MS_DEBUG("attempt: %u | odd port %u is in use, try again", (unsigned int)attempt, (unsigned int)iterate_second_port);

				// If we have tried all the ports in the range raise an error.
				if (iterate_first_port == random_first_port)
				{
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIP);
				}

				continue;
			}

			// Here we already have a theorically available one of two ports.
			// Now let's check whether no other process is listening into them.

			// Set the chosen port(s) into the sockaddr struct(s).
			switch (address_family)
			{
				case AF_INET:
					((struct sockaddr_in*)&first_bind_addr)->sin_port = htons(iterate_first_port);
					if (pair)
						((struct sockaddr_in*)&second_bind_addr)->sin_port = htons(iterate_second_port);
					break;
				case AF_INET6:
					((struct sockaddr_in6*)&first_bind_addr)->sin6_port = htons(iterate_first_port);
					if (pair)
						((struct sockaddr_in6*)&second_bind_addr)->sin6_port = htons(iterate_second_port);
					break;
			}

			// Try to bind on the even address (it may be taken by other process).
			bindAttempt++;

			uvHandles[0] = new uv_tcp_t();

			err = uv_tcp_init(DepLibUV::GetLoop(), uvHandles[0]);
			if (err)
			{
				delete uvHandles[0];
				MS_THROW_ERROR("attempt: %u | uv_tcp_init() failed: %s", (unsigned int)attempt, uv_strerror(err));
			}

			err = uv_tcp_bind(uvHandles[0], (const struct sockaddr*)&first_bind_addr, flags);
			if (err)
			{
				MS_WARN("attempt: %u | uv_tcp_bind() failed for port %u: %s", (unsigned int)attempt, (unsigned int)iterate_first_port, uv_strerror(err));

				uv_close((uv_handle_t*)uvHandles[0], (uv_close_cb)on_error_close);

				// If bind() fails due to "too many open files" stop here.
				if (err == UV_EMFILE)
				{
					MS_THROW_ERROR("bind() fails due to many open files");
				}

				// If bind() fails for more that MAX_BIND_ATTEMPTS then raise an error.
				if (bindAttempt > MAX_BIND_ATTEMPTS)
				{
					MS_THROW_ERROR("bind() fails more than %u times for IP '%s'", MAX_BIND_ATTEMPTS, listenIP);
				}

				// If we have tried all the ports in the range raise an error.
				if (iterate_first_port == random_first_port)
				{
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIP);
				}

				continue;
			}

			// If requested, try to bind also on the odd address.
			if (pair)
			{
				uvHandles[1] = new uv_tcp_t();

				err = uv_tcp_init(DepLibUV::GetLoop(), uvHandles[1]);
				if (err)
				{
					uv_close((uv_handle_t*)uvHandles[0], (uv_close_cb)on_error_close);
					delete uvHandles[1];
					MS_THROW_ERROR("attempt: %u | uv_tcp_init() failed: %s", (unsigned int)attempt, uv_strerror(err));
				}

				err = uv_tcp_bind(uvHandles[1], (const struct sockaddr*)&second_bind_addr, flags);
				if (err)
				{
					MS_WARN("attempt: %u | uv_tcp_bind() failed for odd port %u: %s", (unsigned int)attempt, (unsigned int)iterate_second_port, uv_strerror(err));

					uv_close((uv_handle_t*)uvHandles[0], (uv_close_cb)on_error_close);
					uv_close((uv_handle_t*)uvHandles[1], (uv_close_cb)on_error_close);

					// If bind() fails due to "too many open files" stop here.
					if (err == UV_EMFILE)
					{
						MS_THROW_ERROR("bind() fails due to many open files");
					}

					// If bind() fails for more that MAX_BIND_ATTEMPTS then raise an error.
					if (bindAttempt > MAX_BIND_ATTEMPTS)
					{
						MS_THROW_ERROR("bind() fails more than %u times for IP '%s'", MAX_BIND_ATTEMPTS, listenIP);
					}

					// If we have tried all the ports in the range raise an error.
					if (iterate_first_port == random_first_port)
					{
						MS_THROW_ERROR("no more available ports for IP '%s'", listenIP);
					}

					continue;
				}
			}

			// Success!

			// Set the port(s) as unavailable.
			(*available_ports)[iterate_first_port] = false;
			if (pair)
				(*available_ports)[iterate_second_port] = false;

			if (!pair)
				MS_DEBUG("attempt: %u | available port for IP '%s': %u" , (unsigned int)attempt, listenIP, (unsigned int)iterate_first_port);
			else
				MS_DEBUG("attempt: %u | available ports for IP '%s': %u and %u" , (unsigned int)attempt, listenIP, (unsigned int)iterate_first_port, (unsigned int)iterate_second_port);

			return;
		};
	}

	/* Instance methods. */

	TCPServer::TCPServer(Listener* listener, RTC::TCPConnection::Reader* reader, uv_tcp_t* uvHandle) :
		::TCPServer::TCPServer(uvHandle, 256),
		listener(listener),
		reader(reader)
	{
		MS_TRACE();
	}

	void TCPServer::userOnTCPConnectionAlloc(::TCPConnection** connection)
	{
		MS_TRACE();

		// Allocate a new RTC::TCPConnection for the TCPServer to handle it.
		*connection = new RTC::TCPConnection(this->reader, 65536);
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
