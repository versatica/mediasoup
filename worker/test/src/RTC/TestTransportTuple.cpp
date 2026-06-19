#include "common.hpp"
#include "RTC/PortManager.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <uv.h>
#include <catch2/catch_test_macros.hpp>

SCENARIO("TransportTuple", "[transport-tuple]")
{
	class UdpSocketListener : public RTC::UdpSocket::Listener
	{
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* /*socket*/,
		  const uint8_t* /*data*/,
		  size_t /*len*/,
		  size_t /*bufferLen*/,
		  const struct sockaddr* /*remoteAddr*/) override
		{
		}
	};

	auto makeUdpSocket = [](const std::string& ip, uint16_t minPort, uint16_t maxPort)
	{
		UdpSocketListener listener;
		auto flags = RTC::Transport::SocketFlags{ .ipv6Only = false, .udpReusePort = false };
		RTC::PortManager::PortRangeKey portRangeKey{};
		auto* udpSocket = new RTC::UdpSocket(
		  std::addressof(listener), const_cast<std::string&>(ip), minPort, maxPort, flags, portRangeKey);

		return std::unique_ptr<RTC::UdpSocket>(udpSocket);
	};

	auto makeUdpSockAddr = [](int family, const std::string& ip, uint16_t port)
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
	};

	// We can only make sure that if key1 == key2 then hash(key1) == hash(key2). The opposite is not
	// true since different keys may have the same hash.
	SECTION("2 tuples with same local and remote IP:port have the same hash")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 1234);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 1234);

		RTC::TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		RTC::TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(
		  RTC::TransportTuple::TupleKeyHash{}(udpTuple1.GetTupleKey()) ==
		  RTC::TransportTuple::TupleKeyHash{}(udpTuple2.GetTupleKey()));
		REQUIRE(udpTuple1.GetTupleKey() == udpTuple2.GetTupleKey());
		REQUIRE(udpTuple1.Compare(std::addressof(udpTuple2)));
	}

	SECTION("2 tuples with same local IP:port, same remote IP and different remote port")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10002);

		RTC::TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		RTC::TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.GetTupleKey() != udpTuple2.GetTupleKey());
		REQUIRE(!udpTuple1.Compare(std::addressof(udpTuple2)));

		for (uint16_t remotePort{ 1 }; remotePort < 65535; ++remotePort)
		{
			// SKip if same port as the one in udpRemoteAddr1.
			if (remotePort == 10001)
			{
				continue;
			}

			auto udpRemoteAddr3 = makeUdpSockAddr(AF_INET, "1.2.3.4", remotePort);

			RTC::TransportTuple udpTuple3(udpSocket.get(), udpRemoteAddr3.get());

			REQUIRE(udpTuple1.GetTupleKey() != udpTuple3.GetTupleKey());
			REQUIRE(!udpTuple1.Compare(std::addressof(udpTuple3)));
		}
	}

	SECTION("2 tuples with same local IP:port, different remote IP and same remote port")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 10001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.5", 10001);

		RTC::TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		RTC::TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.GetTupleKey() != udpTuple2.GetTupleKey());
		REQUIRE(!udpTuple1.Compare(std::addressof(udpTuple2)));
	}

	SECTION("2 tuples with same remote IP:port, same local IP and different local port")
	{
		auto udpSocket1     = makeUdpSocket("0.0.0.0", 10000, 20000);
		auto udpSocket2     = makeUdpSocket("0.0.0.0", 30000, 40000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "5.4.3.2", 22222);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "5.4.3.2", 22222);

		RTC::TransportTuple udpTuple1(udpSocket1.get(), udpRemoteAddr1.get());
		RTC::TransportTuple udpTuple2(udpSocket2.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.GetTupleKey() != udpTuple2.GetTupleKey());
		REQUIRE(!udpTuple1.Compare(std::addressof(udpTuple2)));
	}

	SECTION("2 tuples with same local IP:port, same remote IP and different remote port")
	{
		auto udpSocket      = makeUdpSocket("0.0.0.0", 10000, 50000);
		auto udpRemoteAddr1 = makeUdpSockAddr(AF_INET, "1.2.3.4", 40001);
		auto udpRemoteAddr2 = makeUdpSockAddr(AF_INET, "1.2.3.4", 40002);

		RTC::TransportTuple udpTuple1(udpSocket.get(), udpRemoteAddr1.get());
		RTC::TransportTuple udpTuple2(udpSocket.get(), udpRemoteAddr2.get());

		REQUIRE(udpTuple1.GetTupleKey() != udpTuple2.GetTupleKey());
		REQUIRE(!udpTuple1.Compare(std::addressof(udpTuple2)));
	}
}
