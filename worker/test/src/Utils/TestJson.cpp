#include "common.hpp"
#include "Utils.hpp"
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

using namespace Utils;
using json = nlohmann::json;

SCENARIO("Json::IsPositiveInteger()")
{
	json jsonValue;
	int intValue;
	int8_t int8Value;
	int32_t int32Value;
	unsigned int uintValue;
	uint8_t uint8Value;
	uint32_t uint32Value;
	float floatValue;
	json jsonArray{ json::array() };
	json jsonObject{ json::object() };

	jsonValue = 0;
	REQUIRE(Json::IsPositiveInteger(jsonValue));

	jsonValue = 1;
	REQUIRE(Json::IsPositiveInteger(jsonValue));

	jsonValue = 0u;
	REQUIRE(Json::IsPositiveInteger(jsonValue));

	jsonValue = 1u;
	REQUIRE(Json::IsPositiveInteger(jsonValue));

	jsonValue = -1;
	REQUIRE(!Json::IsPositiveInteger(jsonValue));

	intValue = 0;
	REQUIRE(Json::IsPositiveInteger(intValue));

	intValue = 1;
	REQUIRE(Json::IsPositiveInteger(intValue));

	intValue = -1;
	REQUIRE(!Json::IsPositiveInteger(intValue));

	int8Value = 0;
	REQUIRE(Json::IsPositiveInteger(int8Value));

	int8Value = 1;
	REQUIRE(Json::IsPositiveInteger(int8Value));

	int8Value = -1;
	REQUIRE(!Json::IsPositiveInteger(int8Value));

	int32Value = 0;
	REQUIRE(Json::IsPositiveInteger(int32Value));

	int32Value = 1;
	REQUIRE(Json::IsPositiveInteger(int32Value));

	int32Value = -1;
	REQUIRE(!Json::IsPositiveInteger(int32Value));

	uintValue = 0;
	REQUIRE(Json::IsPositiveInteger(uintValue));

	uintValue = 1;
	REQUIRE(Json::IsPositiveInteger(uintValue));

	uint8Value = 0;
	REQUIRE(Json::IsPositiveInteger(uint8Value));

	uint8Value = 1;
	REQUIRE(Json::IsPositiveInteger(uint8Value));

	uint32Value = 0;
	REQUIRE(Json::IsPositiveInteger(uint32Value));

	uint32Value = 1;
	REQUIRE(Json::IsPositiveInteger(uint32Value));

	floatValue = 0;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	floatValue = 0.0;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	floatValue = 1;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	floatValue = 1.1;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	floatValue = -1;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	floatValue = -1.1;
	REQUIRE(!Json::IsPositiveInteger(floatValue));

	REQUIRE(!Json::IsPositiveInteger(jsonArray));
	REQUIRE(!Json::IsPositiveInteger(jsonObject));
	REQUIRE(!Json::IsPositiveInteger(nullptr));
	REQUIRE(!Json::IsPositiveInteger(true));
	REQUIRE(!Json::IsPositiveInteger(false));
}
