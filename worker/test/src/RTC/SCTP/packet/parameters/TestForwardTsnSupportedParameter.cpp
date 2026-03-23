#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Forward-TSN-Supported Parameter (32769)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("ForwardTsnSupportedParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:49152 (FORWARD_TSN_SUPPORTED), Length: 4
			0xC0, 0x00, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = RTC::SCTP::ForwardTsnSupportedParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		delete clonedParameter;
	}

	SECTION("ForwardTsnSupportedParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::ForwardTsnSupportedParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::ForwardTsnSupportedParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		delete parsedParameter;
	}
}
