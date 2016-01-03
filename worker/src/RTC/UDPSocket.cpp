#define MS_CLASS "RTC::UDPSocket"

#include "RTC/UDPSocket.h"
#include "RTC/STUNMessage.h"
#include "RTC/DTLSTransport.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include "Settings.h"
#include "Utils.h"
#include "DepLibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>

#define MAX_BIND_ATTEMPTS 20

/* Static methods for UV callbacks. */

static inline
void on_error_close(uv_handle_t* handle)
{
	delete handle;
}

namespace RTC
{
	/* Class variables. */

	struct sockaddr_storage UDPSocket::sockaddrStorageIPv4;
	struct sockaddr_storage UDPSocket::sockaddrStorageIPv6;
	MS_PORT UDPSocket::minPort;
	MS_PORT UDPSocket::maxPort;
	UDPSocket::AvailablePorts UDPSocket::availableIPv4Ports;
	UDPSocket::AvailablePorts UDPSocket::availableIPv6Ports;

	/* Class methods. */

	void UDPSocket::ClassInit()
	{
		MS_TRACE();

		int err;

		if (Settings::configuration.hasIPv4)
		{
			err = uv_ip4_addr(Settings::configuration.rtcListenIPv4.c_str(), 0, (struct sockaddr_in*)&RTC::UDPSocket::sockaddrStorageIPv4);
			if (err)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));
		}

		if (Settings::configuration.hasIPv6)
		{
			err = uv_ip6_addr(Settings::configuration.rtcListenIPv6.c_str(), 0, (struct sockaddr_in6*)&RTC::UDPSocket::sockaddrStorageIPv6);
			if (err)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
		}

		UDPSocket::minPort = Settings::configuration.rtcMinPort;
		UDPSocket::maxPort = Settings::configuration.rtcMaxPort;

		MS_PORT i = RTC::UDPSocket::minPort;
		do
		{
			RTC::UDPSocket::availableIPv4Ports[i] = true;
			RTC::UDPSocket::availableIPv6Ports[i] = true;
		}
		while (i++ != RTC::UDPSocket::maxPort);
	}

	RTC::UDPSocket* UDPSocket::Factory(Listener* listener, int address_family)
	{
		MS_TRACE();

		uv_udp_t* uvHandles[1];

		// NOTE: This call may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		RandomizePort(address_family, uvHandles, false);

		return new RTC::UDPSocket(listener, uvHandles[0]);
	}

	void UDPSocket::PairFactory(Listener* listener, int address_family, RTC::UDPSocket* sockets[])
	{
		MS_TRACE();

		uv_udp_t* uvHandles[2];

		// NOTE: This call may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		RandomizePort(address_family, uvHandles, true);

		sockets[0] = new RTC::UDPSocket(listener, uvHandles[0]);
		sockets[1] = new RTC::UDPSocket(listener, uvHandles[1]);
	}

	void UDPSocket::RandomizePort(int address_family, uv_udp_t* uvHandles[], bool pair)
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
				available_ports = &RTC::UDPSocket::availableIPv4Ports;
				first_bind_addr = RTC::UDPSocket::sockaddrStorageIPv4;
				second_bind_addr = RTC::UDPSocket::sockaddrStorageIPv4;
				listenIP = Settings::configuration.rtcListenIPv4.c_str();
				break;

			case AF_INET6:
				available_ports = &RTC::UDPSocket::availableIPv6Ports;
				first_bind_addr = RTC::UDPSocket::sockaddrStorageIPv6;
				second_bind_addr = RTC::UDPSocket::sockaddrStorageIPv6;
				listenIP = Settings::configuration.rtcListenIPv6.c_str();
				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_UDP_IPV6ONLY;
				break;

			default:
				MS_THROW_ERROR("invalid address family given");
				break;
		}

		// Choose a random first port to start from.
		random_first_port = (MS_PORT)Utils::Crypto::GetRandomUInt((unsigned int)RTC::UDPSocket::minPort, (unsigned int)RTC::UDPSocket::maxPort);
		// Make it even if pair is requested.
		if (pair)
			random_first_port &= ~1;

		// Set iterate_xxxx_port.
		iterate_first_port = random_first_port;

		// Iterate the RTC UDP ports until get one (or two) available (or fail if not).
		// Fail also after bind() fails N times in theorically available RTC UDP ports.
		while (true)
		{
			attempt++;

			// Increase the iterate port(s) within the range of RTC UDP ports.
			if (!pair)
			{
				if (iterate_first_port < RTC::UDPSocket::maxPort)
					iterate_first_port += 1;
				else
					iterate_first_port = RTC::UDPSocket::minPort;
			}
			else
			{
				if (iterate_first_port < RTC::UDPSocket::maxPort - 1)
					iterate_first_port += 2;
				else
					iterate_first_port = RTC::UDPSocket::minPort;
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
				if (iterate_first_port == random_first_port) {
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIP);
				}

				continue;
			}

			// Check also the odd port (if requested).
			if (pair && !(*available_ports)[iterate_second_port])
			{
				MS_DEBUG("attempt: %u | odd port %u is in use, try again", (unsigned int)attempt, (unsigned int)iterate_second_port);

				// If we have tried all the ports in the range raise an error.
				if (iterate_first_port == random_first_port) {
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

			uvHandles[0] = new uv_udp_t();

			err = uv_udp_init(DepLibUV::GetLoop(), uvHandles[0]);
			if (err)
			{
				delete uvHandles[0];
				MS_THROW_ERROR("attempt: %u | uv_udp_init() failed: %s", (unsigned int)attempt, uv_strerror(err));
			}

			err = uv_udp_bind(uvHandles[0], (const struct sockaddr*)&first_bind_addr, flags);
			if (err)
			{
				MS_WARN("attempt: %u | uv_udp_bind() failed for port %u: %s", (unsigned int)attempt, (unsigned int)iterate_first_port, uv_strerror(err));

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
				uvHandles[1] = new uv_udp_t();

				err = uv_udp_init(DepLibUV::GetLoop(), uvHandles[1]);
				if (err)
				{
					uv_close((uv_handle_t*)uvHandles[0], (uv_close_cb)on_error_close);
					delete uvHandles[1];
					MS_THROW_ERROR("attempt: %u | uv_udp_init() failed: %s", (unsigned int)attempt, uv_strerror(err));
				}

				err = uv_udp_bind(uvHandles[1], (const struct sockaddr*)&second_bind_addr, flags);
				if (err)
				{
					MS_WARN("attempt: %u | uv_udp_bind() failed for odd port %u: %s", (unsigned int)attempt, (unsigned int)iterate_second_port, uv_strerror(err));

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

	UDPSocket::UDPSocket(Listener* listener, uv_udp_t* uvHandle) :
		::UDPSocket::UDPSocket(uvHandle),
		listener(listener)
	{
		MS_TRACE();
	}

	void UDPSocket::userOnUDPDatagramRecv(const MS_BYTE* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_ERROR("no listener set");
			return;
		}

		if (Logger::HasDebugLevel())
		{
			int family;
			MS_PORT port;
			std::string ip;
			Utils::IP::GetAddressInfo(addr, &family, ip, &port);
			MS_DEBUG("%zu bytes datagram received [local: %s : %u | remote: %s : %u]", len, GetLocalIP().c_str(), (unsigned int)GetLocalPort(), ip.c_str(), (unsigned int)port);
		}

		// Check if it's STUN.
		if (STUNMessage::IsSTUN(data, len))
		{
			this->listener->onSTUNDataRecv(this, data, len, addr);
			return;
		}

		// Check if it's RTCP.
		if (RTCPPacket::IsRTCP(data, len))
		{
			this->listener->onRTCPDataRecv(this, data, len, addr);
			return;
		}

		// Check if it's RTP.
		if (RTPPacket::IsRTP(data, len))
		{
			this->listener->onRTPDataRecv(this, data, len, addr);
			return;
		}

		// Check if it's DTLS.
		if (DTLSTransport::IsDTLS(data, len))
		{
			this->listener->onDTLSDataRecv(this, data, len, addr);
			return;
		}

		MS_DEBUG("ignoring received datagram of unknown type");
	}

	void UDPSocket::userOnUDPSocketClosed()
	{
		MS_TRACE();

		// Mark the port as available again.
		if (this->localAddr.ss_family == AF_INET)
			RTC::UDPSocket::availableIPv4Ports[this->localPort] = true;
		else if (this->localAddr.ss_family == AF_INET6)
			RTC::UDPSocket::availableIPv6Ports[this->localPort] = true;
	}
}
