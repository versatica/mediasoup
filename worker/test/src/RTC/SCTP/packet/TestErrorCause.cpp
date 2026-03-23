#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP Error Cause", "[serializable][sctp][errorcause]")
{
	SECTION("alignof() SCTP structs")
	{
		REQUIRE(alignof(RTC::SCTP::ErrorCause::ErrorCauseHeader) == 2);
	}
}
