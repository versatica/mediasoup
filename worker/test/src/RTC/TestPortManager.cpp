#include "common.hpp"
#include "RTC/PortManager.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

// Pre-fix issue #1805: The legacy `GeneratePortRangeHash()` mangled the IPv4
// address with `(address >> 2) << 2`, so any two IPv4 addresses in the same
// /30 block produced the same `uint64_t` hash. The downstream
// `mapPortRanges.find(hash)` then treated them as the same PortRange and
// merged unrelated bindings. This scenario locks down the post-fix
// behavior: distinct tuples produce distinct keys, equal tuples produce equal
// keys.
SCENARIO("PortManager", "[rtc][portmanager]")
{
	// Helper: build an IPv4 `sockaddr_storage` from a dotted-quad string + port=0.
	auto makeV4 = [](const char* dottedQuad)
	{
		sockaddr_storage ss{};
		auto* in = reinterpret_cast<sockaddr_in*>(std::addressof(ss));

		in->sin_family = AF_INET;
		in->sin_port   = 0;

		REQUIRE(inet_pton(AF_INET, dottedQuad, &in->sin_addr) == 1);

		return ss;
	};

	// Helper: build an IPv6 `sockaddr_storage` from a textual address + port=0.
	auto makeV6 = [](const char* literal)
	{
		sockaddr_storage ss{};
		auto* in6 = reinterpret_cast<sockaddr_in6*>(std::addressof(ss));

		in6->sin6_family = AF_INET6;
		in6->sin6_port   = 0;

		REQUIRE(inet_pton(AF_INET6, literal, &in6->sin6_addr) == 1);

		return ss;
	};

	SECTION("identical tuples compare equal and hash equal")
	{
		const auto addr = makeV4("192.168.1.10");

		const RTC::PortManager::PortRangeKey ka(RTC::PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey kb(RTC::PortManager::Protocol::UDP, addr, 40000u, 40099u);

		REQUIRE(ka == kb);
		REQUIRE(RTC::PortManager::PortRangeKeyHash{}(ka) == RTC::PortManager::PortRangeKeyHash{}(kb));
	}

	SECTION("IPv4 addresses in the same /30 are NOT collapsed")
	{
		// 192.168.1.0 / .1 / .2 / .3 all live in 192.168.1.0/30. The old
		// GeneratePortRangeHash dropped the bottom two bits of the address,
		// merging all four into one bucket.
		const auto a0 = makeV4("192.168.1.0");
		const auto a1 = makeV4("192.168.1.1");
		const auto a2 = makeV4("192.168.1.2");
		const auto a3 = makeV4("192.168.1.3");

		const RTC::PortManager::PortRangeKey k0(RTC::PortManager::Protocol::UDP, a0, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey k1(RTC::PortManager::Protocol::UDP, a1, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey k2(RTC::PortManager::Protocol::UDP, a2, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey k3(RTC::PortManager::Protocol::UDP, a3, 40000u, 40099u);

		REQUIRE(k0 != k1);
		REQUIRE(k0 != k2);
		REQUIRE(k0 != k3);
		REQUIRE(k1 != k2);
		REQUIRE(k1 != k3);
		REQUIRE(k2 != k3);
	}

	// The old IPv6 hash folded `a[0] ^ a[1] ^ a[2] ^ a[3]` (4 x uint32_t) into
	// 32 bits. Any two IPv6 addresses where the four 32-bit words XOR to the
	// same value collided. Easiest collision constructor: swap two words.
	// ::1 = 0000:0000:0000:0000:0000:0000:0000:0001 XOR-folds the same as
	// ::1:0:0 (just word reorder).
	SECTION("IPv6 addresses that XOR-fold to the same value are NOT collapsed")
	{
		const auto a = makeV6("::1");
		const auto b = makeV6("1::1:0:0:0");

		const RTC::PortManager::PortRangeKey ka(RTC::PortManager::Protocol::UDP, a, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey kb(RTC::PortManager::Protocol::UDP, b, 40000u, 40099u);

		REQUIRE(ka != kb);
	}

	SECTION("protocol differentiates the key (UDP/TCP on same address+range)")
	{
		const auto addr = makeV4("10.0.0.1");

		const RTC::PortManager::PortRangeKey udp(RTC::PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey tcp(RTC::PortManager::Protocol::TCP, addr, 40000u, 40099u);

		REQUIRE(udp != tcp);
	}

	SECTION("port-range bounds differentiate the key")
	{
		const auto addr = makeV4("10.0.0.1");

		const RTC::PortManager::PortRangeKey ka(RTC::PortManager::Protocol::UDP, addr, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey kb(RTC::PortManager::Protocol::UDP, addr, 40000u, 40100u);
		const RTC::PortManager::PortRangeKey kc(RTC::PortManager::Protocol::UDP, addr, 40001u, 40099u);

		REQUIRE(ka != kb);
		REQUIRE(ka != kc);
		REQUIRE(kb != kc);
	}

	SECTION("family differentiates the key")
	{
		const auto v4 = makeV4("0.0.0.0");
		const auto v6 = makeV6("::");

		const RTC::PortManager::PortRangeKey k4(RTC::PortManager::Protocol::UDP, v4, 40000u, 40099u);
		const RTC::PortManager::PortRangeKey k6(RTC::PortManager::Protocol::UDP, v6, 40000u, 40099u);

		REQUIRE(k4 != k6);
	}
}
