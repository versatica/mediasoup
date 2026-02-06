#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Utils;

SCENARIO("Utils::Time", "[utils][time]")
{
	SECTION("Ntp2TimeMs()")
	{
		const auto nowMs  = DepLibUV::GetTimeMs();
		const auto ntp    = Time::TimeMs2Ntp(nowMs);
		const auto nowMs2 = Time::Ntp2TimeMs(ntp);
		const auto ntp2   = Time::TimeMs2Ntp(nowMs2);

		REQUIRE(nowMs2 == nowMs);
		REQUIRE(ntp2.seconds == ntp.seconds);
		REQUIRE(ntp2.fractions == ntp.fractions);
	}
}
