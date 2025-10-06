#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedAddressTypesParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Supported Address Types Parameter (12)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("SupportedAddressTypesParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:12 (SUPPORTED_ADDRESS_TYPES), Length: 10
			0x00, 0x0C, 0x00, 0x0A,
			// Address Type 1: 0x1001, Address Type 2: 0x2002
			0x10, 0x01, 0x20, 0x02,
			// Address Type 3: 0x3003, 2 bytes of padding
			0x30, 0x03, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = SupportedAddressTypesParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetNumberOfAddressTypes() == 3);
		REQUIRE(parameter->GetAddressTypeAt(0) == 0x1001);
		REQUIRE(parameter->GetAddressTypeAt(1) == 0x2002);
		REQUIRE(parameter->GetAddressTypeAt(2) == 0x3003);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->AddAddressType(1234), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetNumberOfAddressTypes() == 3);
		REQUIRE(parameter->GetAddressTypeAt(0) == 0x1001);
		REQUIRE(parameter->GetAddressTypeAt(1) == 0x2002);
		REQUIRE(parameter->GetAddressTypeAt(2) == 0x3003);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetNumberOfAddressTypes() == 3);
		REQUIRE(clonedParameter->GetAddressTypeAt(0) == 0x1001);
		REQUIRE(clonedParameter->GetAddressTypeAt(1) == 0x2002);
		REQUIRE(clonedParameter->GetAddressTypeAt(2) == 0x3003);

		delete clonedParameter;
	}

	SECTION("SupportedAddressTypesParameter::Parse() fails")
	{
		// Wrong Length field (not even).
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:12 (SUPPORTED_ADDRESS_TYPES), Length: 7
			0x00, 0x0C, 0x00, 0x0A,
			// Address Type 1: 0x1001, Address Type 2: 0x2002
			0x10, 0x01, 0x20, 0x02,
		};
		// clang-format on

		REQUIRE(!SupportedAddressTypesParameter::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("SupportedAddressTypesParameter::Factory() succeeds")
	{
		auto* parameter = SupportedAddressTypesParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetNumberOfAddressTypes() == 0);

		/* Modify it. */

		parameter->AddAddressType(11111);
		parameter->AddAddressType(22222);
		parameter->AddAddressType(33333);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetNumberOfAddressTypes() == 3);
		REQUIRE(parameter->GetAddressTypeAt(0) == 11111);
		REQUIRE(parameter->GetAddressTypeAt(1) == 22222);
		REQUIRE(parameter->GetAddressTypeAt(2) == 33333);

		parameter->AddAddressType(44444);
		parameter->AddAddressType(55555);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetNumberOfAddressTypes() == 5);
		REQUIRE(parameter->GetAddressTypeAt(0) == 11111);
		REQUIRE(parameter->GetAddressTypeAt(1) == 22222);
		REQUIRE(parameter->GetAddressTypeAt(2) == 33333);
		REQUIRE(parameter->GetAddressTypeAt(3) == 44444);
		REQUIRE(parameter->GetAddressTypeAt(4) == 55555);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  SupportedAddressTypesParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 16,
		  /*length*/ 16,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetNumberOfAddressTypes() == 5);
		REQUIRE(parsedParameter->GetAddressTypeAt(0) == 11111);
		REQUIRE(parsedParameter->GetAddressTypeAt(1) == 22222);
		REQUIRE(parsedParameter->GetAddressTypeAt(2) == 33333);
		REQUIRE(parsedParameter->GetAddressTypeAt(3) == 44444);
		REQUIRE(parsedParameter->GetAddressTypeAt(4) == 55555);

		delete parsedParameter;
	}
}
