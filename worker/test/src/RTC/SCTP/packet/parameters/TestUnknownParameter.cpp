#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Unknown Parameter", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnknownParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:49159 (UNKNOWN), Length: 11
			0xC0, 0x07, 0x00, 0x0B,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding
			0x89, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = UnknownParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter->HasUnknownValue() == true);
		REQUIRE(parameter->GetUnknownValueLength() == 7);
		REQUIRE(parameter->GetUnknownValue()[1] == 0x23);
		REQUIRE(parameter->GetUnknownValue()[0] == 0x01);
		REQUIRE(parameter->GetUnknownValue()[1] == 0x23);
		REQUIRE(parameter->GetUnknownValue()[2] == 0x45);
		REQUIRE(parameter->GetUnknownValue()[3] == 0x67);
		REQUIRE(parameter->GetUnknownValue()[4] == 0x89);
		REQUIRE(parameter->GetUnknownValue()[5] == 0xAB);
		REQUIRE(parameter->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(parameter->GetUnknownValue()[7] == 0x00);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter->HasUnknownValue() == true);
		REQUIRE(parameter->GetUnknownValueLength() == 7);
		REQUIRE(parameter->GetUnknownValue()[0] == 0x01);
		REQUIRE(parameter->GetUnknownValue()[1] == 0x23);
		REQUIRE(parameter->GetUnknownValue()[2] == 0x45);
		REQUIRE(parameter->GetUnknownValue()[3] == 0x67);
		REQUIRE(parameter->GetUnknownValue()[4] == 0x89);
		REQUIRE(parameter->GetUnknownValue()[5] == 0xAB);
		REQUIRE(parameter->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(parameter->GetUnknownValue()[7] == 0x00);

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
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(clonedParameter->HasUnknownValue() == true);
		REQUIRE(clonedParameter->GetUnknownValueLength() == 7);
		REQUIRE(clonedParameter->GetUnknownValue()[0] == 0x01);
		REQUIRE(clonedParameter->GetUnknownValue()[1] == 0x23);
		REQUIRE(clonedParameter->GetUnknownValue()[2] == 0x45);
		REQUIRE(clonedParameter->GetUnknownValue()[3] == 0x67);
		REQUIRE(clonedParameter->GetUnknownValue()[4] == 0x89);
		REQUIRE(clonedParameter->GetUnknownValue()[5] == 0xAB);
		REQUIRE(clonedParameter->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(clonedParameter->GetUnknownValue()[7] == 0x00);

		delete clonedParameter;
	}

	SECTION("UnknownParameter::Parse() fails")
	{
		// Wrong Length field.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:49159 (UNKNOWN), Length: 3
			0xC0, 0x07, 0x00, 0x03,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding
			0x89, 0xAB, 0xCD, 0x00,
		};
		// clang-format on

		REQUIRE(!UnknownParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:49159 (UNKNOWN), Length: 11
			0xC0, 0x07, 0x00, 0x0B,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding (missing)
			0x89, 0xAB, 0xCD
		};
		// clang-format on

		REQUIRE(!UnknownParameter::Parse(buffer2, sizeof(buffer2)));
	}
}
