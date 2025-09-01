#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits

using namespace Utils;

SCENARIO("Utils::Number", "[utils][number]")
{
	SECTION("Utils::Number::IsEqualThan()")
	{
		// 0 is not equal than 16.
		REQUIRE(Utils::Number<uint16_t>::IsEqualThan(0, 16) == false);

		// Using N=4 bits, 0 is equal than 16.
		REQUIRE(Utils::Number<uint8_t, 4>::IsEqualThan(0, 16) == true);
		REQUIRE(Utils::Number<uint16_t, 4>::IsEqualThan(0, 16) == true);
		REQUIRE(Utils::Number<uint32_t, 4>::IsEqualThan(0, 16) == true);
		REQUIRE(Utils::Number<uint64_t, 4>::IsEqualThan(0, 16) == true);

		// Using N=7 bits, 0 is equal than 128.
		REQUIRE(Utils::Number<uint8_t, 7>::IsEqualThan(0, 128) == true);
		REQUIRE(Utils::Number<uint16_t, 7>::IsEqualThan(0, 128) == true);
		REQUIRE(Utils::Number<uint32_t, 7>::IsEqualThan(0, 128) == true);
		REQUIRE(Utils::Number<uint64_t, 7>::IsEqualThan(0, 128) == true);
	}

	SECTION("Utils::Number::IsHigherThan()")
	{
		// 10 is higher than std::numeric_limits<uint8_t>::max().
		REQUIRE(Utils::Number<uint8_t>::IsHigherThan(10, std::numeric_limits<uint8_t>::max()) == true);

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

		// Using N=4 bits, 0 is higher than 14.
		REQUIRE(Utils::Number<uint8_t, 4>::IsHigherThan(0, 14) == true);
		REQUIRE(Utils::Number<uint16_t, 4>::IsHigherThan(0, 14) == true);
		REQUIRE(Utils::Number<uint32_t, 4>::IsHigherThan(0, 14) == true);
		REQUIRE(Utils::Number<uint64_t, 4>::IsHigherThan(0, 14) == true);

		// Using N=6 bits, 0 is not higher than 64.
		REQUIRE(Utils::Number<uint8_t, 6>::IsHigherThan(0, 64) == false);
		REQUIRE(Utils::Number<uint16_t, 6>::IsHigherThan(0, 64) == false);
		REQUIRE(Utils::Number<uint32_t, 6>::IsHigherThan(0, 64) == false);
		REQUIRE(Utils::Number<uint64_t, 6>::IsHigherThan(0, 64) == false);
	}

	SECTION("Utils::Number::IsLowerThan()")
	{
		// 1 is lower than 2.
		REQUIRE(Utils::Number<uint8_t>::IsLowerThan(1, 2) == true);

		// std::numeric_limits<uint8_t>::max() is lower than 0.
		REQUIRE(Utils::Number<uint8_t>::IsLowerThan(std::numeric_limits<uint8_t>::max(), 0) == true);

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

		// Using N=3 bits, 7 is lower than 2.
		REQUIRE(Utils::Number<uint8_t, 3>::IsLowerThan(15, 2) == true);
		REQUIRE(Utils::Number<uint16_t, 3>::IsLowerThan(15, 2) == true);
		REQUIRE(Utils::Number<uint32_t, 3>::IsLowerThan(15, 2) == true);
		REQUIRE(Utils::Number<uint64_t, 3>::IsLowerThan(15, 2) == true);

		// Using N=2 bits, 3 is lower than 1.
		REQUIRE(Utils::Number<uint8_t, 2>::IsLowerThan(3, 1) == true);
		REQUIRE(Utils::Number<uint16_t, 2>::IsLowerThan(3, 1) == true);
		REQUIRE(Utils::Number<uint32_t, 2>::IsLowerThan(3, 1) == true);
		REQUIRE(Utils::Number<uint64_t, 2>::IsLowerThan(3, 1) == true);
	}

	SECTION("Utils::Number::IsHigherOrEqualThan()")
	{
		// 0 is greater or equal than std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number<uint64_t>::IsHigherOrEqualThan(0, std::numeric_limits<uint64_t>::max()) == true);

		// Using N=5 bits, 0 is higher or equal than 32.
		REQUIRE(Utils::Number<uint8_t, 5>::IsHigherOrEqualThan(0, 32) == true);
		REQUIRE(Utils::Number<uint16_t, 5>::IsHigherOrEqualThan(0, 32) == true);
		REQUIRE(Utils::Number<uint32_t, 5>::IsHigherOrEqualThan(0, 32) == true);
		REQUIRE(Utils::Number<uint64_t, 5>::IsHigherOrEqualThan(0, 32) == true);
	}

	SECTION("Utils::Number::IsLowerOrEqualThan()")
	{
		// std::numeric_limits<uint64_t>::max() is lower or equal than 0.
		REQUIRE(
		  Utils::Number<uint64_t>::IsLowerOrEqualThan(std::numeric_limits<uint64_t>::max(), 0) == true);

		// Using N=2 bits, 0 is lower or equal than 4.
		REQUIRE(Utils::Number<uint8_t, 2>::IsLowerOrEqualThan(0, 4) == true);
		REQUIRE(Utils::Number<uint16_t, 2>::IsLowerOrEqualThan(0, 4) == true);
		REQUIRE(Utils::Number<uint32_t, 2>::IsLowerOrEqualThan(0, 4) == true);
		REQUIRE(Utils::Number<uint64_t, 2>::IsLowerOrEqualThan(0, 4) == true);

		// Using N=2 bits, 3 is lower or equal than 1.
		REQUIRE(Utils::Number<uint8_t, 2>::IsLowerOrEqualThan(3, 1) == true);
		REQUIRE(Utils::Number<uint16_t, 2>::IsLowerOrEqualThan(3, 1) == true);
		REQUIRE(Utils::Number<uint32_t, 2>::IsLowerOrEqualThan(3, 1) == true);
		REQUIRE(Utils::Number<uint64_t, 2>::IsLowerOrEqualThan(3, 1) == true);
	}
}
