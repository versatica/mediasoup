#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Forward-TSN-Supported Parameter (32769)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ForwardTsnSupportedParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:49152 (FORWARD_TSN_SUPPORTED), Length: 4
			0xC0, 0x00, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = ForwardTsnSupportedParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		delete clonedParameter;
	}

	SECTION("ForwardTsnSupportedParameter::Factory() succeeds")
	{
		auto* parameter = ForwardTsnSupportedParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  ForwardTsnSupportedParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		delete parsedParameter;
	}
}
