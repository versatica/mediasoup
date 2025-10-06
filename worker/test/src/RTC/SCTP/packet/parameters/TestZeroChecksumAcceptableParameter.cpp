#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Zero Checksum Acceptable Parameter (32769)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ZeroChecksumAcceptableParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:32769 (ZERO_CHECKSUM_ACCEPTABLE), Length: 8
			0x80, 0x01, 0x00, 0x08,
			// Alternate Error Detection Method (EDMID) : 666777888
			0x27, 0xBE, 0x39, 0x20,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = ZeroChecksumAcceptableParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666777888));

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(
		  parameter->SetAlternateErrorDetectionMethod(
		    ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS),
		  MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666777888));

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
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  clonedParameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(666777888));

		delete clonedParameter;
	}

	SECTION("ZeroChecksumAcceptableParameter::Factory() succeeds")
	{
		auto* parameter = ZeroChecksumAcceptableParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

		/* Modify it. */

		parameter->SetAlternateErrorDetectionMethod(
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  ZeroChecksumAcceptableParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parsedParameter->GetAlternateErrorDetectionMethod() ==
		  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		delete parsedParameter;
	}
}
