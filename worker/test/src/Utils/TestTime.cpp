#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Utils::Time", "[utils][time]")
{
	SECTION("Ntp2TimeMs()")
	{
		const auto nowMs  = DepLibUV::GetTimeMs();
		const auto ntp    = Utils::Time::TimeMs2Ntp(nowMs);
		const auto nowMs2 = Utils::Time::Ntp2TimeMs(ntp);
		const auto ntp2   = Utils::Time::TimeMs2Ntp(nowMs2);

		REQUIRE(nowMs2 == nowMs);
		REQUIRE(ntp2.seconds == ntp.seconds);
		REQUIRE(ntp2.fractions == ntp.fractions);
	}

	SECTION("TimeMs2Ntp()")
	{
		auto ntp = Utils::Time::TimeMs2Ntp(1500);

		REQUIRE(ntp.seconds == 1);
		// Half a second in NTP fractional units.
		REQUIRE(ntp.fractions == 2147483648);

		// A real NTP instant, seconds since Jan 1, 1900, which still fits in 32 bits.
		ntp = Utils::Time::TimeMs2Ntp(3990000000750);

		REQUIRE(ntp.seconds == 3990000000);
		REQUIRE(Utils::Time::Ntp2TimeMs(ntp) == 3990000000750);
	}

	SECTION("TimeMs2Q32x32()")
	{
		// A whole second is the fractional unit itself.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeMs2Q32x32(1000).value() == 4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeMs2Q32x32(-1000).value() == -4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeMs2Q32x32(0).value() == 0);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeMs2Q32x32(1).value() == 4294967);

		// Seconds are 32 bits wide in the format, so 2^31 seconds no longer fit.
		constexpr int64_t OutOfRangeMs{ (1LL << 31) * 1000 };

		REQUIRE(Utils::Time::TimeMs2Q32x32(OutOfRangeMs) == std::nullopt);
		REQUIRE(Utils::Time::TimeMs2Q32x32(-OutOfRangeMs) == std::nullopt);
		REQUIRE(Utils::Time::TimeMs2Q32x32(OutOfRangeMs - 1).has_value());
		REQUIRE(Utils::Time::TimeMs2Q32x32(-OutOfRangeMs + 1).has_value());
	}

	SECTION("Q32x32ToTimeMs()")
	{
		REQUIRE(Utils::Time::Q32x32ToTimeMs(4294967296) == 1000);
		REQUIRE(Utils::Time::Q32x32ToTimeMs(-4294967296) == -1000);
		REQUIRE(Utils::Time::Q32x32ToTimeMs(0) == 0);

		for (const int64_t ms : { 1, -1, 1000, -1000, 123456, -123456 })
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(Utils::Time::Q32x32ToTimeMs(Utils::Time::TimeMs2Q32x32(ms).value()) == ms);
		}
	}
}
