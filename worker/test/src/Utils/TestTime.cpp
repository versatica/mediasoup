#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Utils;

SCENARIO("Utils::Time", "[utils][time]")
{
	SECTION("Utils::Time::Ntp2TimeMs()")
	{
		auto nowMs  = DepLibUV::GetTimeMs();
		auto ntp    = Time::TimeMs2Ntp(nowMs);
		auto nowMs2 = Time::Ntp2TimeMs(ntp);
		auto ntp2   = Time::TimeMs2Ntp(nowMs2);

		REQUIRE(nowMs2 == nowMs);
		REQUIRE(ntp2.seconds == ntp.seconds);
		REQUIRE(ntp2.fractions == ntp.fractions);
	}

	SECTION("Utils::Time::IsTimeLowerThan()")
	{
		// 1000000 is lower than 2000000.
		REQUIRE(Utils::Time::IsTimeLowerThan(1000000, 2000000) == true);

		// std::numeric_limits<uint64_t>::max() is lower than 0.
		REQUIRE(Utils::Time::IsTimeLowerThan(std::numeric_limits<uint64_t>::max(), 0) == true);

		// (std::numeric_limits<uint64_t>::max() / 2) + 1 is lower than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Time::IsTimeLowerThan(
		    (std::numeric_limits<uint64_t>::max() / 2) + 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is lower than
		// (std::numeric_limits<uint64_t>::max() / 2) - 1.
		REQUIRE(
		  Utils::Time::IsTimeLowerThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) - 1) == true);
	}

	SECTION("Utils::Time::IsTimeHigherThan()")
	{
		// 0 is greater than std::numeric_limits<uint64_t>::max().
		REQUIRE(Utils::Time::IsTimeHigherThan(0, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() / 2) - 1 is higher than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Time::IsTimeHigherThan(
		    (std::numeric_limits<uint64_t>::max() / 2) - 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is higher than
		// (std::numeric_limits<uint64_t>::max() / 2) + 1.
		REQUIRE(
		  Utils::Time::IsTimeHigherThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) + 1) == true);
	}

	SECTION("Utils::Time::IsTimeHigherOrEqualThan()")
	{
		// 0 is greater or equal than std::numeric_limits<uint64_t>::max().
		REQUIRE(Utils::Time::IsTimeHigherOrEqualThan(0, std::numeric_limits<uint64_t>::max()) == true);
	}

	SECTION("Utils::Time::IsTimeLowerOrEqualThan()")
	{
		// std::numeric_limits<uint64_t>::max() is lower or equal than 0.
		REQUIRE(Utils::Time::IsTimeLowerOrEqualThan(std::numeric_limits<uint64_t>::max(), 0) == true);
	}
}
