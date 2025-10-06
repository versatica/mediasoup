#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Incoming SSN Reset Request Parameter (14)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IncomingSsnResetRequestParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:14 (INCOMING_SSN_RESET_REQUEST), Length: 14
			0x00, 0x0E, 0x00, 0x0E,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 0x5001, Stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// Stream 3: 0x5003, 2 bytes of padding
			0x50, 0x03, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = IncomingSsnResetRequestParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetNumberOfStreams() == 3);
		REQUIRE(parameter->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter->GetStreamAt(2) == 0x5003);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetReconfigurationRequestSequenceNumber(111), MediaSoupError);
		REQUIRE_THROWS_AS(parameter->AddStream(444), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetNumberOfStreams() == 3);
		REQUIRE(parameter->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter->GetStreamAt(2) == 0x5003);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(clonedParameter->GetNumberOfStreams() == 3);
		REQUIRE(clonedParameter->GetStreamAt(0) == 0x5001);
		REQUIRE(clonedParameter->GetStreamAt(1) == 0x5002);
		REQUIRE(clonedParameter->GetStreamAt(2) == 0x5003);

		delete clonedParameter;
	}

	SECTION("IncomingSsnResetRequestParameter::Factory() succeeds")
	{
		auto* parameter = IncomingSsnResetRequestParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);
		REQUIRE(parameter->GetNumberOfStreams() == 0);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(111000);
		parameter->AddStream(4444);
		parameter->AddStream(4445);
		parameter->AddStream(4446);
		parameter->AddStream(4447);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parameter->GetNumberOfStreams() == 4);
		REQUIRE(parameter->GetStreamAt(0) == 4444);
		REQUIRE(parameter->GetStreamAt(1) == 4445);
		REQUIRE(parameter->GetStreamAt(2) == 4446);
		REQUIRE(parameter->GetStreamAt(3) == 4447);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  IncomingSsnResetRequestParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 16,
		  /*length*/ 16,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parsedParameter->GetNumberOfStreams() == 4);
		REQUIRE(parsedParameter->GetStreamAt(0) == 4444);
		REQUIRE(parsedParameter->GetStreamAt(1) == 4445);
		REQUIRE(parsedParameter->GetStreamAt(2) == 4446);
		REQUIRE(parsedParameter->GetStreamAt(3) == 4447);

		delete parsedParameter;
	}
}
