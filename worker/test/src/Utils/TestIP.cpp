#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "catch.hpp"

using namespace Utils;

SCENARIO("Utils::IP::GetFamily()")
{
	std::string ip;

	ip = "1.2.3.4";
	REQUIRE(IP::GetFamily(ip) == AF_INET);

	ip = "127.0.0.1";
	REQUIRE(IP::GetFamily(ip) == AF_INET);

	ip = "1::1";
	REQUIRE(IP::GetFamily(ip) == AF_INET6);

	ip = "a:b:c:D::0";
	REQUIRE(IP::GetFamily(ip) == AF_INET6);

	ip = "chicken";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1.2.3.256";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1.2.3.1111";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1.2.3.01";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1::abcde";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1:::";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "1.2.3.4 ";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = " ::1";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);

	ip = "";
	REQUIRE(IP::GetFamily(ip) == AF_UNSPEC);
}

SCENARIO("Utils::IP::NormalizeIp()")
{
	std::string ip;

	ip = "1.2.3.4";
	IP::NormalizeIp(ip);
	REQUIRE(ip == "1.2.3.4");

	ip = "aA::8";
	IP::NormalizeIp(ip);
	REQUIRE(ip == "aa::8");

	ip = "aA::0:0008";
	IP::NormalizeIp(ip);
	REQUIRE(ip == "aa::8");

	ip = "001.2.3.4";
	REQUIRE_THROWS_AS(IP::NormalizeIp(ip), MediaSoupTypeError);

	ip = "1::2::3";
	REQUIRE_THROWS_AS(IP::NormalizeIp(ip), MediaSoupTypeError);

	ip = "::1 ";
	REQUIRE_THROWS_AS(IP::NormalizeIp(ip), MediaSoupTypeError);

	ip = "0.0.0.";
	REQUIRE_THROWS_AS(IP::NormalizeIp(ip), MediaSoupTypeError);

	ip = "";
	REQUIRE_THROWS_AS(IP::NormalizeIp(ip), MediaSoupTypeError);
}
