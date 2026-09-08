#include "common.hpp"
#include "DepLibUV.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("DepLibUV", "[deplibuv]")
{
	SECTION("GetNtpOffsetUs()")
	{
		// Added to our own clock it must give an NTP instant of the current era, so
		// past the moment this test was written.
		//
		// Seconds since Jan 1, 1900 at Jan 1, 2026.
		constexpr int64_t Jan2026NtpSec{ 3976214400 };

		const int64_t ntpUs = DepLibUV::GetTimeUsInt64() + DepLibUV::GetNtpOffsetUs();

		REQUIRE(ntpUs / 1000000 > Jan2026NtpSec);
	}
}
