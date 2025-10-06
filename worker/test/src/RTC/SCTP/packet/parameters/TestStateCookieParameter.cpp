#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("State Cookie Parameter (7)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("StateCookieParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:7 (STATE_COOKIE), Length: 7
			0x00, 0x07, 0x00, 0x07,
			// Cookie: 0xDDCCEE, 1 byte of padding
			0xDD, 0xCC, 0xEE, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = StateCookieParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3);
		REQUIRE(parameter->GetCookie()[0] == 0xDD);
		REQUIRE(parameter->GetCookie()[1] == 0xCC);
		REQUIRE(parameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetCookie()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetCookie(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3);
		REQUIRE(parameter->GetCookie()[0] == 0xDD);
		REQUIRE(parameter->GetCookie()[1] == 0xCC);
		REQUIRE(parameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetCookie()[3] == 0x00);

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
		  /*parameterType*/ Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->HasCookie() == true);
		REQUIRE(clonedParameter->GetCookieLength() == 3);
		REQUIRE(clonedParameter->GetCookie()[0] == 0xDD);
		REQUIRE(clonedParameter->GetCookie()[1] == 0xCC);
		REQUIRE(clonedParameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(clonedParameter->GetCookie()[3] == 0x00);

		delete clonedParameter;
	}

	SECTION("StateCookieParameter::Factory() succeeds")
	{
		auto* parameter = StateCookieParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		/* Modify it. */

		// Verify that replacing the cookie works.
		parameter->SetCookie(DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3000);

		parameter->SetCookie(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasCookie() == false);
		REQUIRE(parameter->GetCookieLength() == 0);

		// 1 bytes + 3 bytes of padding. Note that first (and unique byte) is
		// DataBuffer + 1 which is initialized to 0x0A.
		parameter->SetCookie(DataBuffer + 10, 1);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  StateCookieParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->HasCookie() == true);
		REQUIRE(parsedParameter->GetCookieLength() == 1);
		REQUIRE(parsedParameter->GetCookie()[0] == 0x0A);
		// These should be padding.
		REQUIRE(parsedParameter->GetCookie()[1] == 0x00);
		REQUIRE(parsedParameter->GetCookie()[2] == 0x00);
		REQUIRE(parsedParameter->GetCookie()[3] == 0x00);

		delete parsedParameter;
	}
}
