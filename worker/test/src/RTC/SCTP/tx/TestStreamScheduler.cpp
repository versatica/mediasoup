#include "common.hpp"
#include "RTC/SCTP/tx/StreamScheduler.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP StreamScheduler", "[sctp][streamscheduler]")
{
	// TODO: SCTP: Buffff... a lot.

	constexpr uint64_t Mtu{ 1000 };

	SECTION("has no active streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
	}

	SECTION("can set and get stream properties")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		// TODO: SCTP: Buffff... a lot.
	}
}
