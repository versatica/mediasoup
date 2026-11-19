#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("Incoming SSN Reset Request Parameter (14)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("IncomingSsnResetRequestParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:14 (INCOMING-SSN-RESET-REQUEST), Length: 14
			0x00, 0x0E, 0x00, 0x0E,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 0x5001, stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// Stream 3: 0x5003, 2 bytes of padding
			0x50, 0x03, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::IncomingSsnResetRequestParameter> parameter{
			RTC::SCTP::IncomingSsnResetRequestParameter::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);

		const std::vector<uint16_t> expectedStreamIds{
			{ 0x5001, 0x5002, 0x5003 }
		};

		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Clone it. */

		parameter.reset(parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);
	}

	SECTION("IncomingSsnResetRequestParameter::Parse() fails")
	{
		// Wrong length field (not even).
		// clang-format off
		alignas(4) const uint8_t buffer1[] =
		{
			// Type:14 (INCOMING-SSN-RESET-REQUEST), Length: 13
			0x00, 0x0E, 0x00, 0x0D,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 0x5001, stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// 1 extra byte, 3 bytes of padding
			0x50, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IncomingSsnResetRequestParameter::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("IncomingSsnResetRequestParameter::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::IncomingSsnResetRequestParameter> parameter{
			RTC::SCTP::IncomingSsnResetRequestParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);

		std::vector<uint16_t> expectedStreamIds{};

		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(111000);
		parameter->AddStreamId(4444);
		parameter->AddStreamId(4445);
		parameter->AddStreamId(4446);
		parameter->AddStreamId(4447);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);

		expectedStreamIds = {
			{ 4444, 4445, 4446, 4447 }
		};

		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Parse itself and compare. */

		parameter.reset(
		  RTC::SCTP::IncomingSsnResetRequestParameter::Parse(
		    parameter->GetBuffer(), parameter->GetLength()));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 16,
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);
	}
}
