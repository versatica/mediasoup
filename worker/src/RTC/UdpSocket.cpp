#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV

#include "RTC/UdpSocket.h"
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

	struct sockaddr_storage UdpSocket::sockaddrStorageIPv4;
	struct sockaddr_storage UdpSocket::sockaddrStorageIPv6;
	uint16_t UdpSocket::minPort;
	uint16_t UdpSocket::maxPort;
	std::unordered_map<uint16_t, bool> UdpSocket::availableIPv4Ports;
	std::unordered_map<uint16_t, bool> UdpSocket::availableIPv6Ports;

	/* Class methods. */

	void UdpSocket::ClassInit()
	{
		MS_TRACE();

		int err;

		if (Settings::configuration.hasIPv4)
		{
			err = uv_ip4_addr(Settings::configuration.rtcListenIPv4.c_str(), 0, (struct sockaddr_in*)&RTC::UdpSocket::sockaddrStorageIPv4);
			if (err)
				MS_THROW_ERROR("uv_ipv4_addr() failed: %s", uv_strerror(err));
		}

		if (Settings::configuration.hasIPv6)
		{
			err = uv_ip6_addr(Settings::configuration.rtcListenIPv6.c_str(), 0, (struct sockaddr_in6*)&RTC::UdpSocket::sockaddrStorageIPv6);
			if (err)
				MS_THROW_ERROR("uv_ipv6_addr() failed: %s", uv_strerror(err));
		}

		UdpSocket::minPort = Settings::configuration.rtcMinPort;
		UdpSocket::maxPort = Settings::configuration.rtcMaxPort;

		uint16_t i = RTC::UdpSocket::minPort;
		do
		{
			RTC::UdpSocket::availableIPv4Ports[i] = true;
			RTC::UdpSocket::availableIPv6Ports[i] = true;
		}
		while (i++ != RTC::UdpSocket::maxPort);
	}

	uv_udp_t* UdpSocket::GetRandomPort(int address_family)
	{
		MS_TRACE();

		if (address_family == AF_INET && !Settings::configuration.hasIPv4)
			MS_THROW_ERROR("IPv4 family not available for RTC");
		else if (address_family == AF_INET6 && !Settings::configuration.hasIPv6)
			MS_THROW_ERROR("IPv6 family not available for RTC");

		int err;
		uv_udp_t* uvHandle = nullptr;
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
				available_ports = &RTC::UdpSocket::availableIPv4Ports;
				bind_addr = RTC::UdpSocket::sockaddrStorageIPv4;
				listen_ip = Settings::configuration.rtcListenIPv4.c_str();
				break;

			case AF_INET6:
				available_ports = &RTC::UdpSocket::availableIPv6Ports;
				bind_addr = RTC::UdpSocket::sockaddrStorageIPv6;
				listen_ip = Settings::configuration.rtcListenIPv6.c_str();
				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_UDP_IPV6ONLY;
				break;

			default:
				MS_THROW_ERROR("invalid address family given");
				break;
		}

		// Choose a random port to start from.
		initial_port = (uint16_t)Utils::Crypto::GetRandomUInt((uint32_t)RTC::UdpSocket::minPort, (uint32_t)RTC::UdpSocket::maxPort);

		iterating_port = initial_port;

		// Iterate the RTC UDP ports until getting one available.
		// Fail also after bind() fails N times in theorically available ports.
		while (true)
		{
			++attempt;

			// Increase the iterate port within the range of RTC UDP ports.
			if (iterating_port < RTC::UdpSocket::maxPort)
				++iterating_port;
			else
				iterating_port = RTC::UdpSocket::minPort;

			// Check whether the chosen port is available.
			if (!(*available_ports)[iterating_port])
			{
				MS_DEBUG_DEV("port in use, trying again [port:%" PRIu16 ", attempt:%" PRIu16 "]", iterating_port, attempt);

				// If we have tried all the ports in the range raise an error.
				if (iterating_port == initial_port)
					MS_THROW_ERROR("no more available ports for IP '%s'", listen_ip);

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
			++bind_attempt;

			uvHandle = new uv_udp_t();

			err = uv_udp_init(DepLibUV::GetLoop(), uvHandle);
			if (err)
			{
				delete uvHandle;
				MS_THROW_ERROR("uv_udp_init() failed: %s", uv_strerror(err));
			}

			err = uv_udp_bind(uvHandle, (const struct sockaddr*)&bind_addr, flags);
			if (err)
			{
				MS_WARN_DEV("uv_udp_bind() failed [port:%" PRIu16 ", attempt:%" PRIu16 "]: %s", attempt, iterating_port, uv_strerror(err));

				uv_close((uv_handle_t*)uvHandle, (uv_close_cb)on_error_close);

				// If bind() fails due to "too many open files" stop here.
				if (err == UV_EMFILE)
					MS_THROW_ERROR("uv_udp_bind() fails due to many open files");

				// If bind() fails for more that MAX_BIND_ATTEMPTS then raise an error.
				if (bind_attempt > MAX_BIND_ATTEMPTS)
					MS_THROW_ERROR("uv_udp_bind() fails more than %u times for IP '%s'", MAX_BIND_ATTEMPTS, listen_ip);

				// If we have tried all the ports in the range raise an error.
				if (iterating_port == initial_port)
					MS_THROW_ERROR("no more available ports for IP '%s'", listen_ip);

				continue;
			}

			// Set the port as unavailable.
			(*available_ports)[iterating_port] = false;

			MS_DEBUG_DEV("bind success [ip:%s, port:%" PRIu16 ", attempt:%" PRIu16 "]",
				listen_ip, iterating_port, attempt);

			return uvHandle;
		};
	}

	/* Instance methods. */

	UdpSocket::UdpSocket(Listener* listener, int address_family) :
		// Provide the parent class constructor with a UDP uv handle.
		// NOTE: This may throw a MediaSoupError exception if the address family is not available
		// or there are no available ports.
		::UdpSocket::UdpSocket(GetRandomPort(address_family)),
		listener(listener)
	{
		MS_TRACE();
	}

	void UdpSocket::userOnUdpDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->onPacketRecv(this, data, len, addr);
	}

	void UdpSocket::userOnUdpSocketClosed()
	{
		MS_TRACE();

		// Mark the port as available again.
		if (this->localAddr.ss_family == AF_INET)
			RTC::UdpSocket::availableIPv4Ports[this->localPort] = true;
		else if (this->localAddr.ss_family == AF_INET6)
			RTC::UdpSocket::availableIPv6Ports[this->localPort] = true;
	}
}
