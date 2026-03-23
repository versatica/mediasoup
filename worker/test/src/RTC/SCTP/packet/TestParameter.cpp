#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP Parameter", "[serializable][sctp][parameter]")
{
	SECTION("alignof() SCTP structs")
	{
		REQUIRE(alignof(RTC::SCTP::Parameter::ParameterHeader) == 2);
	}
}
