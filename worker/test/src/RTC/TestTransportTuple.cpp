#include "common.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <uv.h>
#include <catch2/catch_test_macros.hpp>

using namespace RTC;

class UdpSocketListener : public UdpSocket::Listener
{
public:
	void OnUdpSocketPacketReceived(
	  UdpSocket* /*socket*/,
	  const uint8_t* /*data*/,
	  size_t /*len*/,
	  const struct sockaddr* /*remoteAddr*/) override
	{
	}
};

std::unique_ptr<UdpSocket> makeUdpSocket(const std::string& ip, uint16_t minPort, uint16_t maxPort)
{
	UdpSocketListener listener;
	auto flags = Transport::SocketFlags{ .ipv6Only = false, .udpReusePort = false };
	uint64_t portRangeHash{ 0u };
	auto* udpSocket = new UdpSocket(
	  std::addressof(listener), const_cast<std::string&>(ip), minPort, maxPort, flags, portRangeHash);

	return std::unique_ptr<UdpSocket>(udpSocket);
}

std::unique_ptr<sockaddr> makeUdpSockAddr(int family, const std::string& ip, uint16_t port)
{
	if (family == AF_INET)
	{
		auto addr = std::make_unique<sockaddr_in>();

		addr->sin_family = AF_INET;
		addr->sin_port   = htons(port);

		if (uv_inet_pton(AF_INET, ip.c_str(), std::addressof(addr->sin_addr)) != 0)
		{
			throw std::runtime_error("invalid IPv4 address");
		}

		return std::unique_ptr<sockaddr>(reinterpret_cast<sockaddr*>(addr.release()));
	}
	else if (family == AF_INET6)
	{
		auto addr6         = std::make_unique<sockaddr_in6>();
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port   = htons(port);

		if (uv_inet_pton(AF_INET6, ip.c_str(), std::addressof(addr6->sin6_addr)) != 0)
		{
			throw std::runtime_error("invalid IPv6 address");
		}

		return std::unique_ptr<sockaddr>(reinterpret_cast<sockaddr*>(addr6.release()));
	}
	else
	{
		throw std::runtime_error("invalid network family");
	}
}

SCENARIO("TransportTuples have unique hashes", "[transporttuple]")
{
	SECTION("2 tuples with same local and remote IP:port have the same hash")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 1234);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 1234);

		TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.hash == udpTuple2.hash);
	}

	SECTION(
	  "2 tuples with same local IP:port, same remote IP and different remote port have different hashes")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10002);

		TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.hash != udpTuple2.hash);

		for (uint16_t remotePort{ 1 }; remotePort < 65535; ++remotePort)
		{
			// SKip if same port as the one in udpRemoteAddr1.
			if (remotePort == 10001)
			{
				continue;
			}

			auto udpRemoteAddr3 = makeUdpSockAddr(AF_INET, "1.2.3.4", remotePort);

			TransportTuple udpTuple3(udpSocket.get(), udpRemoteAddr3.get());

			REQUIRE(udpTuple1.hash != udpTuple3.hash);
		}
	}

	SECTION(
	  "2 tuples with same local IP:port, different remote IP and same remote port have different hashes")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.5", 10001);

		TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.hash != udpTuple2.hash);
	}

	SECTION(
	  "2 tuples with same remote IP:port, same local IP and different local port have different hashes")
	{
		auto udpSocket1     = makeUdpSocket("0.0.0.0", 10000, 20000);
		auto udpSocket2     = makeUdpSocket("0.0.0.0", 30000, 40000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "5.4.3.2", 22222);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "5.4.3.2", 22222);

		TransportTuple udpTuple1(udpSocket1.get(), udpRemoteAddr1.get());
		TransportTuple udpTuple2(udpSocket2.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.hash != udpTuple2.hash);
	}

	SECTION(
	  "2 tuples with same local IP:port, same remote IP and different remote port have different hashes")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 40001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 40002);

		TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.hash != udpTuple2.hash);
	}
}
