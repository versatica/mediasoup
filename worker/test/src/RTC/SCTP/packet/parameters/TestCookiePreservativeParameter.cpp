#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Cookie Preservative Parameter (9)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("CookiePreservativeParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:9 (COOKIE_PRESERVATIVE), Length: 8
			0x00, 0x09, 0x00, 0x08,
			// Suggested Cookie Life-Span Increment: 4278194466
			0xFF, 0x00, 0x11, 0x22,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = CookiePreservativeParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetLifeSpanIncrement() == 4278194466);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetLifeSpanIncrement(1234), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetLifeSpanIncrement() == 4278194466);

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
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetLifeSpanIncrement() == 4278194466);

		delete clonedParameter;
	}

	SECTION("CookiePreservativeParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:9 (IPV6_ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x08,
			// Suggested Cookie Life-Span Increment: 4278194466
			0xFF, 0x00, 0x11, 0x22,
		};
		// clang-format on

		REQUIRE(!CookiePreservativeParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:9 (COOKIE_PRESERVATIVE), Length: 7
			0x00, 0x09, 0x00, 0x07,
			// Suggested Cookie Life-Span Increment: 4278194466
			0xFF, 0x00, 0x11, 0x22,
		};
		// clang-format on

		REQUIRE(!CookiePreservativeParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Type:9 (COOKIE_PRESERVATIVE), Length: 9
			0x00, 0x09, 0x00, 0x09,
			// Suggested Cookie Life-Span Increment: 4278194466
			0xFF, 0x00, 0x11, 0x22,
			0x69
		};
		// clang-format on

		REQUIRE(!CookiePreservativeParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// Suggested Cookie Life-Span Increment (wrong length)
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!CookiePreservativeParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("CookiePreservativeParameter::Factory() succeeds")
	{
		auto* parameter = CookiePreservativeParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetLifeSpanIncrement() == 0);

		/* Modify it. */

		parameter->SetLifeSpanIncrement(88776655);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetLifeSpanIncrement() == 88776655);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  CookiePreservativeParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetLifeSpanIncrement() == 88776655);

		delete parsedParameter;
	}
}
