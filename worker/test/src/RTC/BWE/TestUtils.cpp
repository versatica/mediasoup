#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/Utils.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE Utils", "[bwe][utils]")
{
	SECTION("ApplyBitrateFactor() gives the bitrate times the factor")
	{
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(600000, 1.02) == 612000);
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(600000, 0.95) == 570000);
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(600000, 1.0) == 600000);
	}

	SECTION("ApplyBitrateFactor() rounds to the nearest bitrate, away from zero on a tie")
	{
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(3, 0.4) == 1);
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(3, 0.5) == 2);
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(7, 0.5) == 4);
	}

	SECTION("ApplyBitrateFactor() gives no bitrate at all beyond what the type holds")
	{
		// Anything over the maximum means no limit, which is what its own value
		// already means.
		REQUIRE(
		  RTC::BWE::Utils::ApplyBitrateFactor(RTC::BWE::Types::BitrateInfinite, 1.3) ==
		  RTC::BWE::Types::BitrateInfinite);
		REQUIRE(
		  RTC::BWE::Utils::ApplyBitrateFactor(RTC::BWE::Types::BitrateInfinite / 2, 4.0) ==
		  RTC::BWE::Types::BitrateInfinite);
		REQUIRE(
		  RTC::BWE::Utils::ApplyBitrateFactor(RTC::BWE::Types::BitrateInfinite, 1.0) ==
		  RTC::BWE::Types::BitrateInfinite);
	}

	SECTION("ApplyBitrateFactor() never gives a negative bitrate")
	{
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(600000, -1.0) == 0);
		REQUIRE(RTC::BWE::Utils::ApplyBitrateFactor(0, 1.5) == 0);
	}

	SECTION("AddBitrates() gives the sum")
	{
		REQUIRE(RTC::BWE::Utils::AddBitrates(600000, 1000) == 601000);
		REQUIRE(RTC::BWE::Utils::AddBitrates(600000, 0) == 600000);
	}

	SECTION("AddBitrates() gives no bitrate at all beyond what the type holds")
	{
		REQUIRE(
		  RTC::BWE::Utils::AddBitrates(RTC::BWE::Types::BitrateInfinite, 1000) ==
		  RTC::BWE::Types::BitrateInfinite);
		REQUIRE(
		  RTC::BWE::Utils::AddBitrates(RTC::BWE::Types::BitrateInfinite - 1000, 1001) ==
		  RTC::BWE::Types::BitrateInfinite);
		REQUIRE(
		  RTC::BWE::Utils::AddBitrates(RTC::BWE::Types::BitrateInfinite - 1000, 1000) ==
		  RTC::BWE::Types::BitrateInfinite);
	}
}
