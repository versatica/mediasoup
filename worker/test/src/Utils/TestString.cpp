#include "common.hpp"
#include "catch.hpp"
#include "Utils.hpp"

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
