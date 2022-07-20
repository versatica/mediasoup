#include "common.hpp"
#include "PayloadChannel/Notification.hpp"
#include <catch2/catch.hpp>

SCENARIO("Notification", "[channel][notification]")
{
	SECTION("notification")
	{
		char foo[]{"n:abcd"};

		REQUIRE(PayloadChannel::Notification::IsNotification(foo, sizeof(foo)) == true);
	}

	SECTION("request")
	{
		char foo[]{"r:abcd"};

		REQUIRE(PayloadChannel::Notification::IsNotification(foo, sizeof(foo)) == false);
	}

	SECTION("non notification")
	{
		char foo[]{"abcd"};

		REQUIRE(PayloadChannel::Notification::IsNotification(foo, sizeof(foo)) == false);
	}

	SECTION("empty string")
	{
		char foo[]{};

		REQUIRE(PayloadChannel::Notification::IsNotification(foo, sizeof(foo)) == false);
	}
}
