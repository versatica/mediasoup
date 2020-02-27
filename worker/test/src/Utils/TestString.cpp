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

SCENARIO("String::Base64Encode()")
{
	std::string data;
	std::string base64;

	data = "abcd";
	base64 = String::Base64Encode(data);
	REQUIRE(base64 == "YWJjZA==");
	REQUIRE(std::string(reinterpret_cast<const char*>(String::Base64Decode(base64))) == data);

	data = "Iñaki";
	base64 = String::Base64Encode(data);
	REQUIRE(base64 == "ScOxYWtp");
	REQUIRE(std::string(reinterpret_cast<const char*>(String::Base64Decode(base64))) == data);
}
