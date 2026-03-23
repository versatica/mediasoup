#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/AddOutgoingStreamsRequestParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Add Outgoing Streams Request Parameter (17)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("AddOutgoingStreamsRequestParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:17 (ADD_OUTGOING_STREAMS_REQUEST), Length: 12
			0x00, 0x11, 0x00, 0x0C,
			// Re-configuration Request Sequence Number: 666777888
			0x27, 0xBE, 0x39, 0x20,
			// Number of new streams: 1024
			0x04, 0x00, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = RTC::SCTP::AddOutgoingStreamsRequestParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(parameter->GetNumberOfNewStreams() == 1024);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(parameter->GetNumberOfNewStreams() == 1024);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationRequestSequenceNumber() == 666777888);
		REQUIRE(clonedParameter->GetNumberOfNewStreams() == 1024);

		delete clonedParameter;
	}

	SECTION("AddOutgoingStreamsRequestParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::AddOutgoingStreamsRequestParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);
		REQUIRE(parameter->GetNumberOfNewStreams() == 0);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(12345678);
		parameter->SetNumberOfNewStreams(2048);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 12345678);
		REQUIRE(parameter->GetNumberOfNewStreams() == 2048);

		/* Parse itself and compare. */

		auto* parsedParameter = RTC::SCTP::AddOutgoingStreamsRequestParameter::Parse(
		  parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationRequestSequenceNumber() == 12345678);
		REQUIRE(parsedParameter->GetNumberOfNewStreams() == 2048);

		delete parsedParameter;
	}
}
