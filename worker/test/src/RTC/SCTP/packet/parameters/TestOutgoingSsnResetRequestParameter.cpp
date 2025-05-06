#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Outgoing SSN Reset Request Parameter (13)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("OutgoingSsnResetRequestParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:13 (OUTGOING_SSN_RESET_REQUEST), Length: 22
			0x00, 0x0D, 0x00, 0x16,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Re-configuration Response Sequence Number: 0x55667788
			0x55, 0x66, 0x77, 0x88,
			// Sender's Last Assigned TSN: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
			// Stream 1: 0x5001, Stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// Stream 3: 0x5003, 2 bytes of padding
			0x50, 0x03, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = OutgoingSsnResetRequestParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter->GetNumberOfStreams() == 3);
		REQUIRE(parameter->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter->GetStreamAt(2) == 0x5003);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetReconfigurationRequestSequenceNumber(111), MediaSoupError);
		REQUIRE_THROWS_AS(parameter->SetReconfigurationResponseSequenceNumber(222), MediaSoupError);
		REQUIRE_THROWS_AS(parameter->SetSenderLastAssignedTsn(333), MediaSoupError);
		REQUIRE_THROWS_AS(parameter->AddStream(444), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);
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
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(clonedParameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(clonedParameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(clonedParameter->GetNumberOfStreams() == 3);
		REQUIRE(clonedParameter->GetStreamAt(0) == 0x5001);
		REQUIRE(clonedParameter->GetStreamAt(1) == 0x5002);
		REQUIRE(clonedParameter->GetStreamAt(2) == 0x5003);

		delete clonedParameter;
	}

	SECTION("OutgoingSsnResetRequestParameter::Factory() succeeds")
	{
		auto* parameter = OutgoingSsnResetRequestParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0);
		REQUIRE(parameter->GetNumberOfStreams() == 0);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(111000);
		parameter->SetReconfigurationResponseSequenceNumber(222000);
		parameter->SetSenderLastAssignedTsn(333000);
		parameter->AddStream(4444);
		parameter->AddStream(4445);
		parameter->AddStream(4446);
		parameter->AddStream(4447);
		parameter->AddStream(4448);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 28,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 222000);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 333000);
		REQUIRE(parameter->GetNumberOfStreams() == 5);
		REQUIRE(parameter->GetStreamAt(0) == 4444);
		REQUIRE(parameter->GetStreamAt(1) == 4445);
		REQUIRE(parameter->GetStreamAt(2) == 4446);
		REQUIRE(parameter->GetStreamAt(3) == 4447);
		REQUIRE(parameter->GetStreamAt(4) == 4448);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  OutgoingSsnResetRequestParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 28,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parsedParameter->GetReconfigurationResponseSequenceNumber() == 222000);
		REQUIRE(parsedParameter->GetSenderLastAssignedTsn() == 333000);
		REQUIRE(parsedParameter->GetNumberOfStreams() == 5);
		REQUIRE(parsedParameter->GetStreamAt(0) == 4444);
		REQUIRE(parsedParameter->GetStreamAt(1) == 4445);
		REQUIRE(parsedParameter->GetStreamAt(2) == 4446);
		REQUIRE(parsedParameter->GetStreamAt(3) == 4447);
		REQUIRE(parsedParameter->GetStreamAt(4) == 4448);

		delete parsedParameter;
	}
}
