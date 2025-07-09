#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/TimeManager.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits()

using namespace RTC;

SCENARIO("TimeManager", "[rtc][TimeManager]")
{
	SECTION("1000000 is lower than 2000000")
	{
		REQUIRE(TimeManager::IsTimeLowerThan(1000000, 2000000) == true);
	}

	SECTION("0 is greater than std::numeric_limits<uint64_t>::max()")
	{
		REQUIRE(TimeManager::IsTimeHigherThan(0, std::numeric_limits<uint64_t>::max()) == true);
	}

	SECTION("0 is greater or equal than std::numeric_limits<uint64_t>::max()")
	{
		REQUIRE(TimeManager::IsTimeHigherOrEqualThan(0, std::numeric_limits<uint64_t>::max()) == true);
	}

	SECTION("std::numeric_limits<uint64_t>::max() is lower than 0")
	{
		REQUIRE(TimeManager::IsTimeLowerThan(std::numeric_limits<uint64_t>::max(), 0) == true);
	}

	SECTION("std::numeric_limits<uint64_t>::max() is lower or equal than 0")
	{
		REQUIRE(TimeManager::IsTimeLowerOrEqualThan(std::numeric_limits<uint64_t>::max(), 0) == true);
	}

	SECTION(
	  "(std::numeric_limits<uint64_t>::max() / 2) + 1 is lower than std::numeric_limits<uint64_t>::max()")
	{
		REQUIRE(
		  TimeManager::IsTimeLowerThan(
		    (std::numeric_limits<uint64_t>::max() / 2) + 1, std::numeric_limits<uint64_t>::max()) == true);
	}

	SECTION(
	  "(std::numeric_limits<uint64_t>::max() / 2) - 1 is higher than std::numeric_limits<uint64_t>::max()")
	{
		REQUIRE(
		  TimeManager::IsTimeHigherThan(
		    (std::numeric_limits<uint64_t>::max() / 2) - 1, std::numeric_limits<uint64_t>::max()) == true);
	}

	SECTION(
	  "std::numeric_limits<uint64_t>::max() is higher than (std::numeric_limits<uint64_t>::max() / 2) + 1")
	{
		REQUIRE(
		  TimeManager::IsTimeHigherThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) + 1) == true);
	}

	SECTION(
	  "std::numeric_limits<uint64_t>::max() is lower than (std::numeric_limits<uint64_t>::max() / 2) - 1")
	{
		REQUIRE(
		  TimeManager::IsTimeLowerThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) - 1) == true);
	}
}
