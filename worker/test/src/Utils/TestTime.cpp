#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include "catch.hpp"

using namespace Utils;

SCENARIO("Utils::Time::TimeMs2Ntp() and Utils::Time::Ntp2TimeMs()")
{
	auto now  = DepLibUV::GetTime();
	auto ntp  = Time::TimeMs2Ntp(now);
	auto now2 = Time::Ntp2TimeMs(ntp);
	auto ntp2 = Time::TimeMs2Ntp(now2);

	REQUIRE(now2 == now);
	REQUIRE(ntp2.seconds == ntp.seconds);
	REQUIRE(ntp2.fractions == ntp.fractions);
}
