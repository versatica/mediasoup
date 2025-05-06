#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Re-configuration Response Parameter (16)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ReconfigurationResponseParameter::Parse() with Sender's and Receiver's Next TSN succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:16 (RECONFIGURATION_RESPONSE), Length: 20
			0x00, 0x10, 0x00, 0x14,
			// Re-configuration Request Sequence Number: 287454020
			0x11, 0x22, 0x33, 0x44,
			// Result: SUCCESS_PERFORMED
			0x00, 0x00, 0x00, 0x01,
			// Sender's Next TSN: 1111111111
			0x42, 0x3A, 0x35, 0xC7,
			// Receiver's Next TSN: 1111111111
			0x84, 0x74, 0x6B, 0x8E,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = ReconfigurationResponseParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(parameter->GetResult() == ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(parameter->GetReceiverNextTsn() == 2222222222);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetReconfigurationResponseSequenceNumber(111), MediaSoupError);
		REQUIRE_THROWS_AS(
		  parameter->SetResult(ReconfigurationResponseParameter::Result::ERROR_WRONG_SSN),
		  MediaSoupError);
		REQUIRE_THROWS_AS(parameter->SetNextTsns(100000000, 200000000), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(parameter->GetResult() == ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(parameter->GetReceiverNextTsn() == 2222222222);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(
		  clonedParameter->GetResult() == ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(clonedParameter->HasNextTsns() == true);
		REQUIRE(clonedParameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(clonedParameter->GetReceiverNextTsn() == 2222222222);

		delete clonedParameter;
	}

	SECTION("ReconfigurationResponseParameter::Parse() without Sender's and Receiver's Next TSN succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:16 (RECONFIGURATION_RESPONSE), Length: 12
			0x00, 0x10, 0x00, 0x0C,
			// Re-configuration Request Sequence Number: 3333333333
			0xC6, 0xAE, 0xA1, 0x55,
			// Result: ERROR_REQUEST_ALREADY_IN_PROGRESS
			0x00, 0x00, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* parameter = ReconfigurationResponseParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  parameter->GetResult() ==
		  ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == false);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetReconfigurationResponseSequenceNumber(111), MediaSoupError);
		REQUIRE_THROWS_AS(
		  parameter->SetResult(ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER),
		  MediaSoupError);
		REQUIRE_THROWS_AS(parameter->SetNextTsns(100000000, 200000000), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  parameter->GetResult() ==
		  ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == false);

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
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  clonedParameter->GetResult() ==
		  ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(clonedParameter->HasNextTsns() == false);

		delete clonedParameter;
	}

	SECTION("ReconfigurationResponseParameter::Parse() fails")
	{
		// Wrong Length field.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:16 (RECONFIGURATION_RESPONSE), Length: 16 (should be 12 or 20)
			0x00, 0x10, 0x00, 0x10,
			// Re-configuration Request Sequence Number: 287454020
			0x11, 0x22, 0x33, 0x44,
			// Result: SUCCESS_PERFORMED
			0x00, 0x00, 0x00, 0x01,
			// Sender's Next TSN: 1111111111
			0x42, 0x3A, 0x35, 0xC7,
			// Receiver's Next TSN: 1111111111
			0x84, 0x74, 0x6B, 0x8E,
		};
		// clang-format on

		REQUIRE(!ReconfigurationResponseParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:16 (RECONFIGURATION_RESPONSE), Length: 20
			0x00, 0x10, 0x00, 0x14,
			// Re-configuration Request Sequence Number: 287454020
			0x11, 0x22, 0x33, 0x44,
			// Result: SUCCESS_PERFORMED
			0x00, 0x00, 0x00, 0x01,
			// Sender's Next TSN: 1111111111
			0x42, 0x3A, 0x35, 0xC7,
			// Receiver's Next TSN (missing)
		};
		// clang-format on

		REQUIRE(!ReconfigurationResponseParameter::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("ReconfigurationResponseParameter::Factory() succeeds")
	{
		auto* parameter = ReconfigurationResponseParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0);
		REQUIRE(parameter->GetResult() == static_cast<ReconfigurationResponseParameter::Result>(0));
		REQUIRE(parameter->HasNextTsns() == false);

		/* Modify it. */

		parameter->SetReconfigurationResponseSequenceNumber(111000);
		parameter->SetResult(ReconfigurationResponseParameter::Result::IN_PROGRESS);
		parameter->SetNextTsns(100000000, 200000000);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 111000);
		REQUIRE(parameter->GetResult() == ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 100000000);
		REQUIRE(parameter->GetReceiverNextTsn() == 200000000);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  ReconfigurationResponseParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationResponseSequenceNumber() == 111000);
		REQUIRE(parsedParameter->GetResult() == ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(parsedParameter->HasNextTsns() == true);
		REQUIRE(parsedParameter->GetSenderNextTsn() == 100000000);
		REQUIRE(parsedParameter->GetReceiverNextTsn() == 200000000);

		delete parsedParameter;
	}
}
