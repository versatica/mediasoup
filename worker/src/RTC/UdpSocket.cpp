#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV

#include "RTC/UdpSocket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include <string>

/* Static methods for UV callbacks. */

static inline void onErrorClose(uv_handle_t* handle)
{
	delete handle;
}

namespace RTC
{
	/* Static. */

	static constexpr uint16_t MaxBindAttempts{ 20 };

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
			err = uv_ip4_addr(
			  Settings::configuration.rtcIPv4.c_str(),
			  0,
			  reinterpret_cast<struct sockaddr_in*>(&RTC::UdpSocket::sockaddrStorageIPv4));

			if (err != 0)
				MS_THROW_ERROR("uv_ipv4_addr() failed: %s", uv_strerror(err));
		}

		if (Settings::configuration.hasIPv6)
		{
			err = uv_ip6_addr(
			  Settings::configuration.rtcIPv6.c_str(),
			  0,
			  reinterpret_cast<struct sockaddr_in6*>(&RTC::UdpSocket::sockaddrStorageIPv6));

			if (err != 0)
				MS_THROW_ERROR("uv_ipv6_addr() failed: %s", uv_strerror(err));
		}

		UdpSocket::minPort = Settings::configuration.rtcMinPort;
		UdpSocket::maxPort = Settings::configuration.rtcMaxPort;

		uint16_t i = RTC::UdpSocket::minPort;

		do
		{
			RTC::UdpSocket::availableIPv4Ports[i] = true;
			RTC::UdpSocket::availableIPv6Ports[i] = true;
		} while (i++ != RTC::UdpSocket::maxPort);
	}

	uv_udp_t* UdpSocket::GetRandomPort(int addressFamily)
	{
		MS_TRACE();

		if (addressFamily == AF_INET && !Settings::configuration.hasIPv4)
			MS_THROW_ERROR("IPv4 family not available for RTC");
		else if (addressFamily == AF_INET6 && !Settings::configuration.hasIPv6)
			MS_THROW_ERROR("IPv6 family not available for RTC");

		int err;
		uv_udp_t* uvHandle{ nullptr };
		// clang-format off
		struct sockaddr_storage bindAddr{};
		// clang-format on
		const char* listenIp;
		uint16_t initialPort;
		uint16_t iteratingPort;
		uint16_t attempt{ 0 };
		uint16_t bindAttempt{ 0 };
		int flags{ 0 };
		std::unordered_map<uint16_t, bool>* availablePorts;

		switch (addressFamily)
		{
			case AF_INET:
				availablePorts = &RTC::UdpSocket::availableIPv4Ports;
				bindAddr       = RTC::UdpSocket::sockaddrStorageIPv4;
				listenIp       = Settings::configuration.rtcIPv4.c_str();
				break;

			case AF_INET6:
				availablePorts = &RTC::UdpSocket::availableIPv6Ports;
				bindAddr       = RTC::UdpSocket::sockaddrStorageIPv6;
				listenIp       = Settings::configuration.rtcIPv6.c_str();
				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_UDP_IPV6ONLY;
				break;

			default:
				MS_THROW_ERROR("invalid address family given");
				break;
		}

		// Choose a random port to start from.
		initialPort = static_cast<uint16_t>(Utils::Crypto::GetRandomUInt(
		  static_cast<uint32_t>(RTC::UdpSocket::minPort), static_cast<uint32_t>(RTC::UdpSocket::maxPort)));

		iteratingPort = initialPort;

		// Iterate the RTC UDP ports until getting one available.
		// Fail also after bind() fails N times in theorically available ports.
		while (true)
		{
			++attempt;

			// Increase the iterate port within the range of RTC UDP ports.
			if (iteratingPort < RTC::UdpSocket::maxPort)
				++iteratingPort;
			else
				iteratingPort = RTC::UdpSocket::minPort;

			// Check whether the chosen port is available.
			if (!(*availablePorts)[iteratingPort])
			{
				MS_DEBUG_DEV(
				  "port in use, trying again [port:%" PRIu16 ", attempt:%" PRIu16 "]", iteratingPort, attempt);

				// If we have tried all the ports in the range raise an error.
				if (iteratingPort == initialPort)
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIp);

				continue;
			}

			// Here we already have a theorically available port.
			// Now let's check whether no other process is listening into it.

			// Set the chosen port into the sockaddr struct(s).
			switch (addressFamily)
			{
				case AF_INET:
					(reinterpret_cast<struct sockaddr_in*>(&bindAddr))->sin_port = htons(iteratingPort);
					break;
				case AF_INET6:
					(reinterpret_cast<struct sockaddr_in6*>(&bindAddr))->sin6_port = htons(iteratingPort);
					break;
			}

			// Try to bind on it.
			++bindAttempt;

			uvHandle = new uv_udp_t();

			err = uv_udp_init(DepLibUV::GetLoop(), uvHandle);
			if (err != 0)
			{
				delete uvHandle;
				MS_THROW_ERROR("uv_udp_init() failed: %s", uv_strerror(err));
			}

			err = uv_udp_bind(uvHandle, reinterpret_cast<const struct sockaddr*>(&bindAddr), flags);
			if (err != 0)
			{
				MS_WARN_DEV(
				  "uv_udp_bind() failed [port:%" PRIu16 ", attempt:%" PRIu16 "]: %s",
				  attempt,
				  iteratingPort,
				  uv_strerror(err));

				uv_close(reinterpret_cast<uv_handle_t*>(uvHandle), static_cast<uv_close_cb>(onErrorClose));

				// If bind() fails due to "too many open files" stop here.
				if (err == UV_EMFILE)
					MS_THROW_ERROR("uv_udp_bind() fails due to many open files");

				// If bind() fails for more that MaxBindAttempts then raise an error.
				if (bindAttempt > MaxBindAttempts)
					MS_THROW_ERROR(
					  "uv_udp_bind() fails more than %" PRIu16 " times for IP '%s'", MaxBindAttempts, listenIp);

				// If we have tried all the ports in the range raise an error.
				if (iteratingPort == initialPort)
					MS_THROW_ERROR("no more available ports for IP '%s'", listenIp);

				continue;
			}

			// Set the port as unavailable.
			(*availablePorts)[iteratingPort] = false;

			MS_DEBUG_DEV(
			  "bind success [ip:%s, port:%" PRIu16 ", attempt:%" PRIu16 "]", listenIp, iteratingPort, attempt);

			return uvHandle;
		};
	}

	/* Instance methods. */

	UdpSocket::UdpSocket(Listener* listener, int addressFamily)
	  : // Provide the parent class constructor with a UDP uv handle.
	    // NOTE: This may throw a MediaSoupError exception if the address family is not available
	    // or there are no available ports.
	    ::UdpSocket::UdpSocket(GetRandomPort(addressFamily)), listener(listener)
	{
		MS_TRACE();
	}

	void UdpSocket::UserOnUdpDatagramRecv(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (this->listener == nullptr)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->OnPacketRecv(this, data, len, addr);
	}

	void UdpSocket::UserOnUdpSocketClosed()
	{
		MS_TRACE();

		// Mark the port as available again.
		if (this->localAddr.ss_family == AF_INET)
			RTC::UdpSocket::availableIPv4Ports[this->localPort] = true;
		else if (this->localAddr.ss_family == AF_INET6)
			RTC::UdpSocket::availableIPv6Ports[this->localPort] = true;
	}
} // namespace RTC
