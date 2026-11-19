#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
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
			// Type:49152 (FORWARD-TSN-SUPPORTED), Length: 4
			0xC0, 0x00, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::ForwardTsnSupportedParameter> parameter{
			RTC::SCTP::ForwardTsnSupportedParameter::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
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
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Clone it. */

		parameter.reset(parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);
	}

	SECTION("ForwardTsnSupportedParameter::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::ForwardTsnSupportedParameter> parameter{
			RTC::SCTP::ForwardTsnSupportedParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		/* Parse itself and compare. */

		parameter.reset(
		  RTC::SCTP::ForwardTsnSupportedParameter::Parse(parameter->GetBuffer(), parameter->GetLength()));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);
	}
}
