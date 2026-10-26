#include "common.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::abs()
#include <limits>

SCENARIO("Utils::Time", "[utils][time]")
{
	SECTION("TimeUsToMs()")
	{
		REQUIRE(Utils::Time::TimeUsToMs(0) == 0);
		REQUIRE(Utils::Time::TimeUsToMs(1000) == 1);
		REQUIRE(Utils::Time::TimeUsToMs(1499) == 1);
		// Halves upwards.
		REQUIRE(Utils::Time::TimeUsToMs(1500) == 2);
		REQUIRE(Utils::Time::TimeUsToMs(1501) == 2);
		REQUIRE(Utils::Time::TimeUsToMs(3990000000750000) == 3990000000750);

		// A negative instant rounds the same way, so halves still go upwards rather
		// than towards zero.
		REQUIRE(Utils::Time::TimeUsToMs(-1000) == -1);
		REQUIRE(Utils::Time::TimeUsToMs(-1499) == -1);
		REQUIRE(Utils::Time::TimeUsToMs(-1500) == -1);
		REQUIRE(Utils::Time::TimeUsToMs(-1501) == -2);
		REQUIRE(Utils::Time::TimeUsToMs(-500) == 0);
		REQUIRE(Utils::Time::TimeUsToMs(-501) == -1);

		// The extremes are rounded rather than overflowing.
		REQUIRE(Utils::Time::TimeUsToMs(std::numeric_limits<int64_t>::max()) == 9223372036854776LL);
		REQUIRE(Utils::Time::TimeUsToMs(std::numeric_limits<int64_t>::min()) == -9223372036854776LL);
	}

	SECTION("NtpToTimeUs()")
	{
		const auto nowUs  = DepLibUV::GetTimeUs();
		const auto ntp    = Utils::Time::TimeUsToNtp(nowUs);
		const auto nowUs2 = Utils::Time::NtpToTimeUs(ntp);
		const auto ntp2   = Utils::Time::TimeUsToNtp(nowUs2);

		REQUIRE(nowUs2 == nowUs);
		REQUIRE(ntp2.seconds == ntp.seconds);
		REQUIRE(ntp2.fractions == ntp.fractions);
	}

	SECTION("TimeUsToNtp()")
	{
		auto ntp = Utils::Time::TimeUsToNtp(1500000);

		REQUIRE(ntp.seconds == 1);
		// Half a second in NTP fractional units.
		REQUIRE(ntp.fractions == 2147483648);

		// A real NTP instant, seconds since Jan 1, 1900, which still fits in 32 bits.
		ntp = Utils::Time::TimeUsToNtp(3990000000750000);

		REQUIRE(ntp.seconds == 3990000000);
		REQUIRE(Utils::Time::NtpToTimeUs(ntp) == 3990000000750000);

		// Sub-millisecond times are kept.
		REQUIRE(Utils::Time::TimeUsToNtp(1000500).fractions > Utils::Time::TimeUsToNtp(1000000).fractions);
		REQUIRE(Utils::Time::TimeUsToNtp(1000500).fractions < Utils::Time::TimeUsToNtp(1001000).fractions);
		REQUIRE(Utils::Time::NtpToTimeUs(Utils::Time::TimeUsToNtp(1000500)) == 1000500);
	}

	// Middle 32 bits of the given NTP timestamp.
	const auto toCompactNtp = [](uint32_t seconds, uint32_t fractions) -> uint32_t
	{
		return (seconds << 16) | (fractions >> 16);
	};

	const auto toTimeUs = [](uint32_t seconds, uint32_t fractions) -> int64_t
	{
		return Utils::Time::NtpToTimeUs(Utils::Time::Ntp{ .seconds = seconds, .fractions = fractions });
	};

	SECTION("CompactNtpIntervalToTimeUs()")
	{
		{
			const int64_t diffUs = toTimeUs(0x12654, 0x64335) - toTimeUs(0x12345, 0x23456);
			const uint32_t compactNtpDiff = toCompactNtp(0x12654, 0x64335) - toCompactNtp(0x12345, 0x23456);

			REQUIRE(std::abs(Utils::Time::CompactNtpIntervalToTimeUs(compactNtpDiff) - diffUs) <= 1000);
		}

		// The later timestamp has the lower compact NTP representation, which is fine as
		// long as the difference is computed with unsigned arithmetic.
		{
			const int64_t diffUs = toTimeUs(0x20000, 0x64335) - toTimeUs(0x1ffff, 0x23456);

			REQUIRE(diffUs > 0);
			REQUIRE(toCompactNtp(0x20000, 0x64335) < toCompactNtp(0x1ffff, 0x23456));

			const uint32_t compactNtpDiff = toCompactNtp(0x20000, 0x64335) - toCompactNtp(0x1ffff, 0x23456);

			REQUIRE(std::abs(Utils::Time::CompactNtpIntervalToTimeUs(compactNtpDiff) - diffUs) <= 1000);
		}

		// A difference close to 2^16 seconds is a negative one.
		{
			const int64_t diffUs = toTimeUs(0x1ffff, 0x64335) - toTimeUs(0x20000, 0x23456);

			REQUIRE(diffUs < 0);

			const uint32_t compactNtpDiff = toCompactNtp(0x1ffff, 0x64335) - toCompactNtp(0x20000, 0x23456);

			REQUIRE(std::abs(Utils::Time::CompactNtpIntervalToTimeUs(compactNtpDiff) - diffUs) <= 1000);
		}

		// Right in the middle, both +2^15 and -2^15 seconds are valid results.
		REQUIRE(std::abs(Utils::Time::CompactNtpIntervalToTimeUs(0x80000000)) == 0x8000 * 1000000LL);
	}

	SECTION("CompactNtpRttToTimeUs()")
	{
		// A difference close to 2^15 seconds is still a positive one.
		{
			const int64_t diffUs = toTimeUs(0x17fff, 0xffff5) - toTimeUs(0x10000, 0x00006);

			REQUIRE(std::abs(diffUs - (((1 << 15) - 1) * 1000000LL)) <= 1000);

			const uint32_t compactNtpDiff = toCompactNtp(0x17fff, 0xffff5) - toCompactNtp(0x10000, 0x00006);

			REQUIRE(std::abs(Utils::Time::CompactNtpRttToTimeUs(compactNtpDiff) - diffUs) <= 1000);
		}

		// A negative round trip time yields 1 millisecond.
		{
			const int64_t diffUs = toTimeUs(0x1ffff, 0x64335) - toTimeUs(0x20000, 0x23456);

			REQUIRE(diffUs < 0);

			const uint32_t compactNtpDiff = toCompactNtp(0x1ffff, 0x64335) - toCompactNtp(0x20000, 0x23456);

			REQUIRE(Utils::Time::CompactNtpRttToTimeUs(compactNtpDiff) == 1000);
		}
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

	SECTION("TimeUsToQ32x32()")
	{
		// A whole second is the fractional unit itself.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUsToQ32x32(1000000).value() == 4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUsToQ32x32(-1000000).value() == -4294967296);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUsToQ32x32(0).value() == 0);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(Utils::Time::TimeUsToQ32x32(1000).value() == 4294967);

		// Seconds are 32 bits wide in the format, so 2^31 seconds no longer fit.
		constexpr int64_t OutOfRangeUs{ (1LL << 31) * 1000000 };

		REQUIRE(Utils::Time::TimeUsToQ32x32(OutOfRangeUs) == std::nullopt);
		REQUIRE(Utils::Time::TimeUsToQ32x32(-OutOfRangeUs) == std::nullopt);
		REQUIRE(Utils::Time::TimeUsToQ32x32(OutOfRangeUs - 1).has_value());
		REQUIRE(Utils::Time::TimeUsToQ32x32(-OutOfRangeUs + 1).has_value());
	}

	SECTION("Q32x32ToTimeUs()")
	{
		REQUIRE(Utils::Time::Q32x32ToTimeUs(4294967296) == 1000000);
		REQUIRE(Utils::Time::Q32x32ToTimeUs(-4294967296) == -1000000);
		REQUIRE(Utils::Time::Q32x32ToTimeUs(0) == 0);

		for (const int64_t us : { 1, -1, 1000, -1000, 1000000, -1000000, 123456789, -123456789 })
		{
			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			REQUIRE(Utils::Time::Q32x32ToTimeUs(Utils::Time::TimeUsToQ32x32(us).value()) == us);
		}
	}
}
