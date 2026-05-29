#include "common.hpp"
#include "RTC/PortManager.hpp"
#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

namespace
{
	// Helper: build an IPv4 sockaddr_storage from a dotted-quad string + port=0.
	sockaddr_storage MakeV4(const char* dottedQuad)
	{
		sockaddr_storage ss{};
		auto* in = reinterpret_cast<sockaddr_in*>(&ss);
		in->sin_family = AF_INET;
		in->sin_port   = 0;
		REQUIRE(inet_pton(AF_INET, dottedQuad, &in->sin_addr) == 1);

		return ss;
	}

	// Helper: build an IPv6 sockaddr_storage from a textual address + port=0.
	sockaddr_storage MakeV6(const char* literal)
	{
		sockaddr_storage ss{};
		auto* in6 = reinterpret_cast<sockaddr_in6*>(&ss);
		in6->sin6_family = AF_INET6;
		in6->sin6_port   = 0;
		REQUIRE(inet_pton(AF_INET6, literal, &in6->sin6_addr) == 1);

		return ss;
	}
} // namespace

SCENARIO("PortManager::PortRangeKey - exact-tuple semantics (issue #1805)", "[rtc][port-manager]")
{
	using RTC::PortManager;

	// Pre-fix issue #1805: the legacy GeneratePortRangeHash mangled the IPv4
	// address with `(address >> 2) << 2`, so any two IPv4 addresses in the
	// same /30 block produced the same uint64_t hash. The downstream
	// `mapPortRanges.find(hash)` then treated them as the same PortRange and
	// merged unrelated bindings. This scenario locks down the post-fix
	// behavior: distinct tuples produce distinct keys, equal tuples produce
	// equal keys.

	SECTION("identical tuples compare equal and hash equal")
	{
		const auto addr = MakeV4("192.168.1.10");

		const PortManager::PortRangeKey a(
		  PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const PortManager::PortRangeKey b(
		  PortManager::Protocol::UDP, addr, 40000u, 40099u);

		REQUIRE(a == b);
		REQUIRE(PortManager::PortRangeKeyHash{}(a) == PortManager::PortRangeKeyHash{}(b));
	}

	SECTION("IPv4 addresses in the same /30 are NOT collapsed (was #1805)")
	{
		// 192.168.1.0 / .1 / .2 / .3 all live in 192.168.1.0/30. The old
		// GeneratePortRangeHash dropped the bottom two bits of the address,
		// merging all four into one bucket.
		const auto a0 = MakeV4("192.168.1.0");
		const auto a1 = MakeV4("192.168.1.1");
		const auto a2 = MakeV4("192.168.1.2");
		const auto a3 = MakeV4("192.168.1.3");

		const PortManager::PortRangeKey k0(PortManager::Protocol::UDP, a0, 40000u, 40099u);
		const PortManager::PortRangeKey k1(PortManager::Protocol::UDP, a1, 40000u, 40099u);
		const PortManager::PortRangeKey k2(PortManager::Protocol::UDP, a2, 40000u, 40099u);
		const PortManager::PortRangeKey k3(PortManager::Protocol::UDP, a3, 40000u, 40099u);

		REQUIRE(k0 != k1);
		REQUIRE(k0 != k2);
		REQUIRE(k0 != k3);
		REQUIRE(k1 != k2);
		REQUIRE(k1 != k3);
		REQUIRE(k2 != k3);
	}

	SECTION("IPv6 addresses that XOR-fold to the same value are NOT collapsed")
	{
		// The old IPv6 hash folded `a[0] ^ a[1] ^ a[2] ^ a[3]` (4 x uint32_t)
		// into 32 bits. Any two IPv6 addresses where the four 32-bit words
		// XOR to the same value collided. Easiest collision constructor: swap
		// two words. ::1 = 0000:0000:0000:0000:0000:0000:0000:0001 XOR-folds
		// the same as ::1:0:0 (just word reorder).
		const auto a = MakeV6("::1");
		const auto b = MakeV6("1::1:0:0:0");

		const PortManager::PortRangeKey ka(PortManager::Protocol::UDP, a, 40000u, 40099u);
		const PortManager::PortRangeKey kb(PortManager::Protocol::UDP, b, 40000u, 40099u);

		REQUIRE(ka != kb);
	}

	SECTION("Protocol differentiates the key (UDP/TCP on same address+range)")
	{
		const auto addr = MakeV4("10.0.0.1");

		const PortManager::PortRangeKey udp(
		  PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const PortManager::PortRangeKey tcp(
		  PortManager::Protocol::TCP, addr, 40000u, 40099u);

		REQUIRE(udp != tcp);
	}

	SECTION("Port-range bounds differentiate the key")
	{
		const auto addr = MakeV4("10.0.0.1");

		const PortManager::PortRangeKey a(
		  PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const PortManager::PortRangeKey b(
		  PortManager::Protocol::UDP, addr, 40000u, 40100u);
		const PortManager::PortRangeKey c(
		  PortManager::Protocol::UDP, addr, 40001u, 40099u);

		REQUIRE(a != b);
		REQUIRE(a != c);
		REQUIRE(b != c);
	}

	SECTION("Family differentiates the key")
	{
		const auto v4   = MakeV4("0.0.0.0");
		const auto v6   = MakeV6("::");

		const PortManager::PortRangeKey k4(
		  PortManager::Protocol::UDP, v4, 40000u, 40099u);
		const PortManager::PortRangeKey k6(
		  PortManager::Protocol::UDP, v6, 40000u, 40099u);

		REQUIRE(k4 != k6);
	}
}
