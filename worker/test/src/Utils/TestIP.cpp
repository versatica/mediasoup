#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Utils::IP", "[utils][ip]")
{
	SECTION("GetFamily()")
	{
		std::string ip;

		ip = "1.2.3.4";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET);

		ip = "127.0.0.1";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET);

		ip = "255.255.255.255";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET);

		ip = "1::1";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET6);

		ip = "a:b:c:D::0";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET6);

		ip = "0000:0000:0000:0000:0000:ffff:192.168.100.228";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_INET6);

		ip = "::0:";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "3::3:1:";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "chicken";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1.2.3.256";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1.2.3.1111";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1.2.3.01";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1::abcde";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1:::";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "1.2.3.4 ";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = " ::1";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);

		ip = "0000:0000:0000:0000:0000:ffff:192.168.100.228.4567";
		REQUIRE(Utils::IP::GetFamily(ip) == AF_UNSPEC);
	}

	SECTION("NormalizeIp()")
	{
		std::string ip;

		ip = "1.2.3.4";
		Utils::IP::NormalizeIp(ip);
		REQUIRE(ip == "1.2.3.4");

		ip = "255.255.255.255";
		Utils::IP::NormalizeIp(ip);
		REQUIRE(ip == "255.255.255.255");

		ip = "aA::8";
		Utils::IP::NormalizeIp(ip);
		REQUIRE(ip == "aa::8");

		ip = "aA::0:0008";
		Utils::IP::NormalizeIp(ip);
		REQUIRE(ip == "aa::8");

		ip = "001.2.3.4";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "0255.255.255.255";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "1::2::3";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "::1 ";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "0.0.0.";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "::0:";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "3::3:1:";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);

		ip = "";
		REQUIRE_THROWS_AS(Utils::IP::NormalizeIp(ip), MediaSoupTypeError);
	}

	SECTION("GetAddressInfo()")
	{
		struct sockaddr_in sin{};

		std::memset(&sin, 0, sizeof(sin));

		sin.sin_family      = AF_INET;
		sin.sin_port        = htons(10251);
		sin.sin_addr.s_addr = inet_addr("82.99.219.114");

		const auto* addr = reinterpret_cast<const struct sockaddr*>(&sin);
		int family;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(addr, family, ip, port);

		REQUIRE(family == AF_INET);
		REQUIRE(ip == "82.99.219.114");
		REQUIRE(port == 10251);
	}
}
