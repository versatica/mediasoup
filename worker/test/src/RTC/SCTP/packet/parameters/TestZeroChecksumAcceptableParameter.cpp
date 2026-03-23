#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Zero Checksum Acceptable Parameter (32769)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("ZeroChecksumAcceptableParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* parameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(
		    666777888));

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(
		    666777888));

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  clonedParameter->GetAlternateErrorDetectionMethod() ==
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(
		    666777888));

		delete clonedParameter;
	}

	SECTION("ZeroChecksumAcceptableParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

		/* Modify it. */

		parameter->SetAlternateErrorDetectionMethod(
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		/* Parse itself and compare. */

		auto* parsedParameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Parse(
		  parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(
		  parsedParameter->GetAlternateErrorDetectionMethod() ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		delete parsedParameter;
	}
}
