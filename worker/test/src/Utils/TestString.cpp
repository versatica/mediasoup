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
	size_t outLen;
	unsigned char* encodedPtr;
	unsigned char* decodedPtr;
	std::string encoded;
	std::string decoded;

	data       = "abcd";
	encodedPtr = String::Base64Encode(data, &outLen);
	encoded    = std::string(reinterpret_cast<char*>(encodedPtr), outLen - 1); // TODO
	decodedPtr = String::Base64Decode(encoded, &outLen);
	decoded    = std::string(reinterpret_cast<char*>(decodedPtr), outLen);

	REQUIRE(encoded == "YWJjZA==");
	REQUIRE(decoded == data);

	data       = "Iñaki";
	encodedPtr = String::Base64Encode(data, &outLen);
	encoded    = std::string(reinterpret_cast<char*>(encodedPtr), outLen - 1); // TODO
	decodedPtr = String::Base64Decode(encoded, &outLen);
	decoded    = std::string(reinterpret_cast<char*>(decodedPtr), outLen);

	REQUIRE(encoded == "ScOxYWtp");
	REQUIRE(decoded == data);
}
