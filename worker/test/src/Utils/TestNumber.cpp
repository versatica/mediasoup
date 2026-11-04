#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits

SCENARIO("Utils::Number", "[utils][number]")
{
	SECTION("MaxValue()")
	{
		// A length of 0 means every bit of T.
		REQUIRE(Utils::Number::maxValue<uint8_t>(0) == std::numeric_limits<uint8_t>::max());
		REQUIRE(Utils::Number::maxValue<uint16_t>(0) == std::numeric_limits<uint16_t>::max());
		REQUIRE(Utils::Number::maxValue<uint32_t>(0) == std::numeric_limits<uint32_t>::max());
		REQUIRE(Utils::Number::maxValue<uint64_t>(0) == std::numeric_limits<uint64_t>::max());

		// A length as wide as T also means every bit of it.
		REQUIRE(Utils::Number::maxValue<uint8_t>(8) == std::numeric_limits<uint8_t>::max());
		REQUIRE(Utils::Number::maxValue<uint16_t>(16) == std::numeric_limits<uint16_t>::max());
		REQUIRE(Utils::Number::maxValue<uint32_t>(32) == std::numeric_limits<uint32_t>::max());
		REQUIRE(Utils::Number::maxValue<uint64_t>(64) == std::numeric_limits<uint64_t>::max());

		// So does a length wider than T, which no value of it could ever take.
		REQUIRE(Utils::Number::maxValue<uint8_t>(9) == std::numeric_limits<uint8_t>::max());
		REQUIRE(Utils::Number::maxValue<uint16_t>(17) == std::numeric_limits<uint16_t>::max());
		REQUIRE(Utils::Number::maxValue<uint32_t>(33) == std::numeric_limits<uint32_t>::max());
		REQUIRE(Utils::Number::maxValue<uint64_t>(65) == std::numeric_limits<uint64_t>::max());

		// The widest value of T still fits one bit below its own width.
		REQUIRE(Utils::Number::maxValue<uint32_t>(31) == 2147483647);
		REQUIRE(Utils::Number::maxValue<uint64_t>(63) == 9223372036854775807);

		REQUIRE(Utils::Number::maxValue<uint16_t>(14) == 16383);
		REQUIRE(Utils::Number::maxValue<uint16_t>(15) == 32767);
		REQUIRE(Utils::Number::maxValue<uint32_t>(15) == 32767);
	}

	SECTION("IsEqualThan()")
	{
		// 0 is not equal than 16.
		REQUIRE(Utils::Number::IsEqualThan<uint16_t>(0, 16) == false);

		// Using N=4 bits, 0 is equal than 16.
		REQUIRE(Utils::Number::IsEqualThan<uint8_t, 4>(0, 16) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint16_t, 4>(0, 16) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint32_t, 4>(0, 16) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint64_t, 4>(0, 16) == true);

		// Using N=7 bits, 0 is equal than 128.
		REQUIRE(Utils::Number::IsEqualThan<uint8_t, 7>(0, 128) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint16_t, 7>(0, 128) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint32_t, 7>(0, 128) == true);
		REQUIRE(Utils::Number::IsEqualThan<uint64_t, 7>(0, 128) == true);
	}

	SECTION("IsHigherThan()")
	{
		// 10 is higher than std::numeric_limits<uint8_t>::max().
		REQUIRE(Utils::Number::IsHigherThan<uint8_t>(10, std::numeric_limits<uint8_t>::max()) == true);

		// 0 is greater than std::numeric_limits<uint64_t>::max().
		REQUIRE(Utils::Number::IsHigherThan<uint64_t>(0, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() / 2) - 1 is higher than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number::IsHigherThan<uint64_t>(
		    (std::numeric_limits<uint64_t>::max() / 2) - 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is higher than
		// (std::numeric_limits<uint64_t>::max() / 2) + 1.
		REQUIRE(
		  Utils::Number::IsHigherThan<uint64_t>(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) + 1) == true);

		// Using N=4 bits, 0 is higher than 14.
		REQUIRE(Utils::Number::IsHigherThan<uint8_t, 4>(0, 14) == true);
		REQUIRE(Utils::Number::IsHigherThan<uint16_t, 4>(0, 14) == true);
		REQUIRE(Utils::Number::IsHigherThan<uint32_t, 4>(0, 14) == true);
		REQUIRE(Utils::Number::IsHigherThan<uint64_t, 4>(0, 14) == true);

		// Using N=6 bits, 0 is not higher than 64.
		REQUIRE(Utils::Number::IsHigherThan<uint8_t, 6>(0, 64) == false);
		REQUIRE(Utils::Number::IsHigherThan<uint16_t, 6>(0, 64) == false);
		REQUIRE(Utils::Number::IsHigherThan<uint32_t, 6>(0, 64) == false);
		REQUIRE(Utils::Number::IsHigherThan<uint64_t, 6>(0, 64) == false);
	}

	SECTION("IsHigherThan() with max value")
	{
		// Same as giving the bits it takes.
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(0, 14, Utils::Number::maxValue<uint16_t>(4)) == true);
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(0, 64, Utils::Number::maxValue<uint16_t>(6)) == false);

		// Using 16 bits, 5 is higher than 65530 since the latter has rolled over.
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(5, 65530, Utils::Number::maxValue<uint16_t>(16)) == true);

		// 20000 is higher than 5 using 16 bits, but not using 15, where it is far
		// enough to be taken as rolled over instead.
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(20000, 5, Utils::Number::maxValue<uint16_t>(16)) == true);
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(20000, 5, Utils::Number::maxValue<uint16_t>(15)) == false);

		// Any other length is honored the very same way, 14 bits here.
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(10000, 5, Utils::Number::maxValue<uint16_t>(15)) == true);
		REQUIRE(
		  Utils::Number::IsHigherThan<uint16_t>(10000, 5, Utils::Number::maxValue<uint16_t>(14)) == false);
	}

	SECTION("IsLowerThan()")
	{
		// 1 is lower than 2.
		REQUIRE(Utils::Number::IsLowerThan<uint8_t>(1, 2) == true);

		// std::numeric_limits<uint8_t>::max() is lower than 0.
		REQUIRE(Utils::Number::IsLowerThan<uint8_t>(std::numeric_limits<uint8_t>::max(), 0) == true);

		// 1000000 is lower than 2000000.
		REQUIRE(Utils::Number::IsLowerThan<uint64_t>(1000000, 2000000) == true);

		// std::numeric_limits<uint64_t>::max() is lower than 0.
		REQUIRE(Utils::Number::IsLowerThan<uint64_t>(std::numeric_limits<uint64_t>::max(), 0) == true);

		// (std::numeric_limits<uint64_t>::max() / 2) + 1 is lower than
		// std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number::IsLowerThan<uint64_t>(
		    (std::numeric_limits<uint64_t>::max() / 2) + 1, std::numeric_limits<uint64_t>::max()) == true);

		// std::numeric_limits<uint64_t>::max() is lower than
		// (std::numeric_limits<uint64_t>::max() / 2) - 1.
		REQUIRE(
		  Utils::Number::IsLowerThan<uint64_t>(
		    std::numeric_limits<uint64_t>::max(), (std::numeric_limits<uint64_t>::max() / 2) - 1) == true);

		// Using N=3 bits, 7 is lower than 2.
		REQUIRE(Utils::Number::IsLowerThan<uint8_t, 3>(15, 2) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint16_t, 3>(15, 2) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint32_t, 3>(15, 2) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint64_t, 3>(15, 2) == true);

		// Using N=2 bits, 3 is lower than 1.
		REQUIRE(Utils::Number::IsLowerThan<uint8_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint16_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint32_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerThan<uint64_t, 2>(3, 1) == true);
	}

	SECTION("IsLowerThan() with max value")
	{
		// Same as giving the bits it takes.
		REQUIRE(Utils::Number::IsLowerThan<uint16_t>(15, 2, Utils::Number::maxValue<uint16_t>(3)) == true);

		// Using 16 bits, 65530 is lower than 5 since the latter has rolled over.
		REQUIRE(
		  Utils::Number::IsLowerThan<uint16_t>(65530, 5, Utils::Number::maxValue<uint16_t>(16)) == true);

		// 5 is lower than 20000 using 16 bits, but not using 15, where the latter
		// is far enough to be taken as rolled over instead.
		REQUIRE(
		  Utils::Number::IsLowerThan<uint16_t>(5, 20000, Utils::Number::maxValue<uint16_t>(16)) == true);
		REQUIRE(
		  Utils::Number::IsLowerThan<uint16_t>(5, 20000, Utils::Number::maxValue<uint16_t>(15)) == false);

		// Any other length is honored the very same way, 14 bits here.
		REQUIRE(
		  Utils::Number::IsLowerThan<uint16_t>(5, 10000, Utils::Number::maxValue<uint16_t>(15)) == true);
		REQUIRE(
		  Utils::Number::IsLowerThan<uint16_t>(5, 10000, Utils::Number::maxValue<uint16_t>(14)) == false);
	}

	SECTION("IsHigherOrEqualThan()")
	{
		// 0 is greater or equal than std::numeric_limits<uint64_t>::max().
		REQUIRE(
		  Utils::Number::IsHigherOrEqualThan<uint64_t>(0, std::numeric_limits<uint64_t>::max()) == true);

		// Using N=5 bits, 0 is higher or equal than 32.
		REQUIRE(Utils::Number::IsHigherOrEqualThan<uint8_t, 5>(0, 32) == true);
		REQUIRE(Utils::Number::IsHigherOrEqualThan<uint16_t, 5>(0, 32) == true);
		REQUIRE(Utils::Number::IsHigherOrEqualThan<uint32_t, 5>(0, 32) == true);
		REQUIRE(Utils::Number::IsHigherOrEqualThan<uint64_t, 5>(0, 32) == true);
	}

	SECTION("IsLowerOrEqualThan()")
	{
		// std::numeric_limits<uint64_t>::max() is lower or equal than 0.
		REQUIRE(
		  Utils::Number::IsLowerOrEqualThan<uint64_t>(std::numeric_limits<uint64_t>::max(), 0) == true);

		// Using N=2 bits, 0 is lower or equal than 4.
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint8_t, 2>(0, 4) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint16_t, 2>(0, 4) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint32_t, 2>(0, 4) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint64_t, 2>(0, 4) == true);

		// Using N=2 bits, 3 is lower or equal than 1.
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint8_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint16_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint32_t, 2>(3, 1) == true);
		REQUIRE(Utils::Number::IsLowerOrEqualThan<uint64_t, 2>(3, 1) == true);
	}

	SECTION("ForwardDiff()")
	{
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff(4711u, 4711u)) == 0);

		uint8_t x{ 0 };
		uint8_t y{ 255 };

		for (uint16_t i{ 0 }; i < 256; ++i)
		{
			REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff(x, y)) == 255);

			++x;
			++y;
		}

		uint32_t yi{ 255 };

		for (uint16_t i{ 0 }; i < 512; ++i)
		{
			REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t>(x, yi)) == 255);

			++x;
			++yi;
		}
	}

	SECTION("ForwardDiff() with divisor")
	{
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t, 123>(0, 122)) == 122);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t, 123>(122, 122)) == 0);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t, 123>(1, 0)) == 122);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t, 123>(0, 0)) == 0);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ForwardDiff<uint8_t, 123>(122, 0)) == 1);
	}

	SECTION("ReverseDiff()")
	{
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff(4711u, 4711u)) == 0);

		uint8_t x{ 0 };
		uint8_t y{ 255 };

		for (uint16_t i{ 0 }; i < 256; ++i)
		{
			REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff(x, y)) == 1);

			++x;
			++y;
		}

		uint32_t yi{ 255 };

		for (uint16_t i{ 0 }; i < 512; ++i)
		{
			REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t>(x, yi)) == 1);

			++x;
			++yi;
		}
	}

	SECTION("ReverseDiff() with divisor")
	{
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t, 123>(0, 122)) == 1);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t, 123>(122, 122)) == 0);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t, 123>(1, 0)) == 1);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t, 123>(0, 0)) == 0);
		REQUIRE(static_cast<uint64_t>(Utils::Number::ReverseDiff<uint8_t, 123>(122, 0)) == 122);
	}
}
