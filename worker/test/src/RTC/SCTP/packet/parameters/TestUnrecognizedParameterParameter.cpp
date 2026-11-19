#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unrecognized Parameter Parameter (7)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("UnrecognizedParameterParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:8 (UNRECOGNIZED-PARAMETER), Length: 7
			0x00, 0x08, 0x00, 0x07,
			// Unrecognized Parameter: 0xDDCCEE, 1 byte of padding
			0xDD, 0xCC, 0xEE, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::UnrecognizedParameterParameter> parameter{
			RTC::SCTP::UnrecognizedParameterParameter::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);

		/* Clone it. */

		parameter.reset(parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0xDD);
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0xCC);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);
	}

	SECTION("UnrecognizedParameterParameter::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::UnrecognizedParameterParameter> parameter{
			RTC::SCTP::UnrecognizedParameterParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		/* Modify it. */

		// Verify that replacing the unrecognized parameter works.
		parameter->SetUnrecognizedParameter(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 3000);

		parameter->SetUnrecognizedParameter(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasUnrecognizedParameter() == false);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 0);

		// 1 bytes + 3 bytes of padding. Note that first (and unique byte) is
		// sctpCommon::DataBuffer + 1 which is initialized to 0x0A.
		parameter->SetUnrecognizedParameter(sctpCommon::DataBuffer + 10, 1);

		/* Parse itself and compare. */

		parameter.reset(
		  RTC::SCTP::UnrecognizedParameterParameter::Parse(
		    parameter->GetBuffer(), parameter->GetLength()));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasUnrecognizedParameter() == true);
		REQUIRE(parameter->GetUnrecognizedParameterLength() == 1);
		REQUIRE(parameter->GetUnrecognizedParameter()[0] == 0x0A);
		// These should be padding.
		REQUIRE(parameter->GetUnrecognizedParameter()[1] == 0x00);
		REQUIRE(parameter->GetUnrecognizedParameter()[2] == 0x00);
		REQUIRE(parameter->GetUnrecognizedParameter()[3] == 0x00);
	}
}
