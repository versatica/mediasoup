#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Utils;

SCENARIO("Utils::Number", "[utils][number]")
{
	SECTION("Utils::Number<uint64_t>::IsLowerThan()")
	{
		// 1000000 is lower than 2000000.
		REQUIRE(Utils::Number<uint64_t>::IsLowerThan(1000000, 2000000) == true);

		// std::numeric_limits<uint64_t>::max() is lower than 0.
		REQUIRE(Utils::Number<uint64_t>::IsLowerThan(std::numeric_limits<uint64_t>::max(), 0) == true);

		// (std::numeric_limits<uint64_t>::max() / 2) + 1 is lower than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number<uint64_t>::IsLowerThan(
		    (std::numeric_limits<uint64_t>::max() / 2) + 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is lower than
		// (std::numeric_limits<uint64_t>::max() / 2) - 1.
		REQUIRE(
		  Utils::Number<uint64_t>::IsLowerThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) - 1) == true);
	}

	SECTION("Utils::Number<uint64_t>::IsHigherThan()")
	{
		// 0 is greater than std::numeric_limits<uint64_t>::max().
		REQUIRE(Utils::Number<uint64_t>::IsHigherThan(0, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() / 2) - 1 is higher than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number<uint64_t>::IsHigherThan(
		    (std::numeric_limits<uint64_t>::max() / 2) - 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is higher than
		// (std::numeric_limits<uint64_t>::max() / 2) + 1.
		REQUIRE(
		  Utils::Number<uint64_t>::IsHigherThan(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) + 1) == true);
	}

	SECTION("Utils::Number<uint64_t>::IsLowerOrEqualThan()")
	{
		// std::numeric_limits<uint64_t>::max() is lower or equal than 0.
		REQUIRE(
		  Utils::Number<uint64_t>::IsLowerOrEqualThan(std::numeric_limits<uint64_t>::max(), 0) == true);
	}

	SECTION("Utils::Number<uint64_t>::IsHigherOrEqualThan()")
	{
		// 0 is greater or equal than std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number<uint64_t>::IsHigherOrEqualThan(0, std::numeric_limits<uint64_t>::max()) == true);
	}
}
