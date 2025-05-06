#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/AddIncomingStreamsRequestParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Add Incoming Streams Request Parameter (18)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("AddIncomingStreamsRequestParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:18 (ADD_INCOMING_STREAMS_REQUEST), Length: 12
			0x00, 0x12, 0x00, 0x0C,
			// Re-configuration Request Sequence Number: 666777888
			0x27, 0xBE, 0x39, 0x20,
			// Number of new streams: 1024
			0x04, 0x00, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = AddIncomingStreamsRequestParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(parameter->GetNumberOfNewStreams() == 1024);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetReconfigurationRequestSequenceNumber(1234), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(parameter->GetNumberOfNewStreams() == 1024);

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
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(clonedParameter->GetNumberOfNewStreams() == 1024);

		delete clonedParameter;
	}

	SECTION("AddIncomingStreamsRequestParameter::Factory() succeeds")
	{
		auto* parameter =
		  AddIncomingStreamsRequestParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);
		REQUIRE(parameter->GetNumberOfNewStreams() == 0);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(12345678);
		parameter->SetNumberOfNewStreams(2048);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 12345678);
		REQUIRE(parameter->GetNumberOfNewStreams() == 2048);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  AddIncomingStreamsRequestParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationRequestSequenceNumber() == 12345678);
		REQUIRE(parsedParameter->GetNumberOfNewStreams() == 2048);

		delete parsedParameter;
	}
}
