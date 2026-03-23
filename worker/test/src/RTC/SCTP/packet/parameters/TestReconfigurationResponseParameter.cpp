#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Re-configuration Response Parameter (16)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("ReconfigurationResponseParameter::Parse() with Sender's and Receiver's Next TSN succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* parameter = RTC::SCTP::ReconfigurationResponseParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(
		  parameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(parameter->GetReceiverNextTsn() == 2222222222);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(
		  parameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(parameter->GetReceiverNextTsn() == 2222222222);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationResponseSequenceNumber() == 287454020);
		REQUIRE(
		  clonedParameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		REQUIRE(clonedParameter->HasNextTsns() == true);
		REQUIRE(clonedParameter->GetSenderNextTsn() == 1111111111);
		REQUIRE(clonedParameter->GetReceiverNextTsn() == 2222222222);

		delete clonedParameter;
	}

	SECTION("ReconfigurationResponseParameter::Parse() without Sender's and Receiver's Next TSN succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* parameter = RTC::SCTP::ReconfigurationResponseParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  parameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == false);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  parameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == false);

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
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetReconfigurationResponseSequenceNumber() == 3333333333);
		REQUIRE(
		  clonedParameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS);
		REQUIRE(clonedParameter->HasNextTsns() == false);

		delete clonedParameter;
	}

	SECTION("ReconfigurationResponseParameter::Parse() fails")
	{
		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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

		REQUIRE(!RTC::SCTP::ReconfigurationResponseParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
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

		REQUIRE(!RTC::SCTP::ReconfigurationResponseParameter::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("ReconfigurationResponseParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::ReconfigurationResponseParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0);
		REQUIRE(
		  parameter->GetResult() == static_cast<RTC::SCTP::ReconfigurationResponseParameter::Result>(0));
		REQUIRE(parameter->HasNextTsns() == false);

		/* Modify it. */

		parameter->SetReconfigurationResponseSequenceNumber(111000);
		parameter->SetResult(RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		parameter->SetNextTsns(100000000, 200000000);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 111000);
		REQUIRE(
		  parameter->GetResult() == RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(parameter->HasNextTsns() == true);
		REQUIRE(parameter->GetSenderNextTsn() == 100000000);
		REQUIRE(parameter->GetReceiverNextTsn() == 200000000);

		/* Parse itself and compare. */

		auto* parsedParameter = RTC::SCTP::ReconfigurationResponseParameter::Parse(
		  parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetReconfigurationResponseSequenceNumber() == 111000);
		REQUIRE(
		  parsedParameter->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(parsedParameter->HasNextTsns() == true);
		REQUIRE(parsedParameter->GetSenderNextTsn() == 100000000);
		REQUIRE(parsedParameter->GetReceiverNextTsn() == 200000000);

		delete parsedParameter;
	}
}
