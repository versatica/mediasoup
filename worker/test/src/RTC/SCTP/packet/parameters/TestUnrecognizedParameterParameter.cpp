#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Unrecognized Parameter Parameter (7)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnrecognizedParameterParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:8 (UNRECOGNIZED_PARAMETER), Length: 7
			0x00, 0x08, 0x00, 0x07,
			// Unrecognized Parameter: 0xDDCCEE, 1 byte of padding
			0xDD, 0xCC, 0xEE, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = UnrecognizedParameterParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetUnrecognizedParameter(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->HasUnrecognizedParameter() == true);
		REQUIRE(clonedParameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(clonedParameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(clonedParameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(clonedParameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(clonedParameter->GetUnrecognizedParameter()[3] == 0x00);

		delete clonedParameter;
	}

	SECTION("UnrecognizedParameterParameter::Factory() succeeds")
	{
		auto* parameter = UnrecognizedParameterParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		/* Modify it. */

		// Verify that replacing the unrecognized parameter works.
		parameter->SetUnrecognizedParameter(DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3000);

		parameter->SetUnrecognizedParameter(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasUnrecognizedParameter() == false);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 0);

		// 1 bytes + 3 bytes of padding. Note that first (and unique byte) is
		// DataBuffer + 1 which is initialized to 0x0A.
		parameter->SetUnrecognizedParameter(DataBuffer + 10, 1);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  UnrecognizedParameterParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->HasUnrecognizedParameter() == true);
		REQUIRE(parsedParameter->GetUnrecognizedParameterLength() == 1);
		REQUIRE(parsedParameter->GetUnrecognizedParameter()[0] == 0x0A);
		// These should be padding.
		REQUIRE(parsedParameter->GetUnrecognizedParameter()[1] == 0x00);
		REQUIRE(parsedParameter->GetUnrecognizedParameter()[2] == 0x00);
		REQUIRE(parsedParameter->GetUnrecognizedParameter()[3] == 0x00);

		delete parsedParameter;
	}
}
