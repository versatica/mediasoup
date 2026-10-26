#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
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
			// Type:32769 (ZERO-CHECKSUM-ACCEPTABLE), Length: 8
			0x80, 0x01, 0x00, 0x08,
			// Alternate Error Detection Method (EDMID) : 0x00000001
			0x00, 0x00, 0x00, 0x01,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* const parameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Parse(buffer, sizeof(buffer));

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
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

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
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		/* Clone it. */

		auto* const clonedParameter =
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
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS);

		delete clonedParameter;
	}

	SECTION("ZeroChecksumAcceptableParameter::GetAlternateErrorDetectionMethod() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:32769 (ZERO-CHECKSUM-ACCEPTABLE), Length: 8
			0x80, 0x01, 0x00, 0x08,
			// Alternate Error Detection Method (EDMID) : 0xFFFFFFFE
			0xFF, 0xFF, 0xFF, 0xFE
		};
		// clang-format on

		auto* const parameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Parse(buffer, sizeof(buffer));

		REQUIRE(parameter);
		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::TRUSTED_NETWORK);

		// Unknown Alternate Error Detection Method (0x00000002).
		parameter->SetAlternateErrorDetectionMethod(
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  static_cast<RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod>(
		    0x00000002));

		REQUIRE(
		  parameter->GetAlternateErrorDetectionMethod() ==
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE);

		delete parameter;
	}

	SECTION("ZeroChecksumAcceptableParameter::Factory() succeeds")
	{
		auto* const parameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Factory(
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

		auto* const parsedParameter = RTC::SCTP::ZeroChecksumAcceptableParameter::Parse(
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
