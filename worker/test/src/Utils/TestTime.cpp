#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Utils::Time", "[utils][time]")
{
	SECTION("Ntp2TimeUs()")
	{
		const auto nowUs  = DepLibUV::GetTimeUsInt64();
		const auto ntp    = Utils::Time::TimeUs2Ntp(nowUs);
		const auto nowUs2 = Utils::Time::Ntp2TimeUs(ntp);
		const auto ntp2   = Utils::Time::TimeUs2Ntp(nowUs2);

		REQUIRE(nowUs2 == nowUs);
		REQUIRE(ntp2.seconds == ntp.seconds);
		REQUIRE(ntp2.fractions == ntp.fractions);
	}

	SECTION("TimeUs2Ntp()")
	{
		auto ntp = Utils::Time::TimeUs2Ntp(1500000);

		REQUIRE(ntp.seconds == 1);
		// Half a second in NTP fractional units.
		REQUIRE(ntp.fractions == 2147483648);

		// A real NTP instant, seconds since Jan 1, 1900, which still fits in 32 bits.
		ntp = Utils::Time::TimeUs2Ntp(3990000000750000);

		REQUIRE(ntp.seconds == 3990000000);
		REQUIRE(Utils::Time::Ntp2TimeUs(ntp) == 3990000000750000);

		// Sub-millisecond times are kept.
		REQUIRE(Utils::Time::TimeUs2Ntp(1000500).fractions > Utils::Time::TimeUs2Ntp(1000000).fractions);
		REQUIRE(Utils::Time::TimeUs2Ntp(1000500).fractions < Utils::Time::TimeUs2Ntp(1001000).fractions);
		REQUIRE(Utils::Time::Ntp2TimeUs(Utils::Time::TimeUs2Ntp(1000500)) == 1000500);
	}

	SECTION("TimeUsToAbsSendTime()")
	{
		// A whole second is the fractional unit itself, being the format 6.18 fixed
		// point seconds.
		REQUIRE(Utils::Time::TimeUsToAbsSendTime(1000000) == 262144);
		REQUIRE(Utils::Time::TimeUsToAbsSendTime(1500000) == 393216);
		REQUIRE(Utils::Time::TimeUsToAbsSendTime(0) == 0);
		// Resolution is 1/262144 of a second, so a couple of microseconds already
		// move the value.
		REQUIRE(Utils::Time::TimeUsToAbsSendTime(2) == 1);

		// Only 6 bits of seconds are kept, so the value wraps every 64 seconds.
		constexpr int64_t WrapPeriodUs{ 64 * 1000000 };

		REQUIRE(Utils::Time::TimeUsToAbsSendTime(WrapPeriodUs) == 0);
		REQUIRE(
		  Utils::Time::TimeUsToAbsSendTime(WrapPeriodUs + 1000000) ==
		  Utils::Time::TimeUsToAbsSendTime(1000000));

		// A negative time yields the value of the positive time it's congruent with.
		REQUIRE(
		  Utils::Time::TimeUsToAbsSendTime(-1000000) ==
		  Utils::Time::TimeUsToAbsSendTime(WrapPeriodUs - 1000000));
		REQUIRE(Utils::Time::TimeUsToAbsSendTime(-WrapPeriodUs) == 0);
	}

	SECTION("TimeUs2Q32x32()")
	{
		// A whole second is the fractional unit itself.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUs2Q32x32(1000000).value() == 4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUs2Q32x32(-1000000).value() == -4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUs2Q32x32(0).value() == 0);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUs2Q32x32(1000).value() == 4294967);

		// Seconds are 32 bits wide in the format, so 2^31 seconds no longer fit.
		constexpr int64_t OutOfRangeUs{ (1LL << 31) * 1000000 };

		REQUIRE(Utils::Time::TimeUs2Q32x32(OutOfRangeUs) == std::nullopt);
		REQUIRE(Utils::Time::TimeUs2Q32x32(-OutOfRangeUs) == std::nullopt);
		REQUIRE(Utils::Time::TimeUs2Q32x32(OutOfRangeUs - 1).has_value());
		REQUIRE(Utils::Time::TimeUs2Q32x32(-OutOfRangeUs + 1).has_value());
	}

	SECTION("Q32x32ToTimeUs()")
	{
		REQUIRE(Utils::Time::Q32x32ToTimeUs(4294967296) == 1000000);
		REQUIRE(Utils::Time::Q32x32ToTimeUs(-4294967296) == -1000000);
		REQUIRE(Utils::Time::Q32x32ToTimeUs(0) == 0);

		for (const int64_t us : { 1, -1, 1000, -1000, 1000000, -1000000, 123456789, -123456789 })
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(Utils::Time::Q32x32ToTimeUs(Utils::Time::TimeUs2Q32x32(us).value()) == us);
		}
	}
}
