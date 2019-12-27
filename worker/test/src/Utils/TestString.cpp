#include "common.hpp"
#include "Utils.hpp"
#include <catch.hpp>

using namespace Utils;

SCENARIO("String::ToLowerCase()")
{
	std::string str;

	str = "Foo";
	String::ToLowerCase(str);
	REQUIRE(str == "foo");

	str = "Foo!œ";
	String::ToLowerCase(str);
	REQUIRE(str == "foo!œ");
}
