#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("Outgoing SSN Reset Request Parameter (13)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("OutgoingSsnResetRequestParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:13 (OUTGOING-SSN-RESET-REQUEST), Length: 22
			0x00, 0x0D, 0x00, 0x16,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Re-configuration Response Sequence Number: 0x55667788
			0x55, 0x66, 0x77, 0x88,
			// Sender's Last Assigned TSN: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
			// Stream 1: 0x5001, stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// Stream 3: 0x5003, 2 bytes of padding
			0x50, 0x03, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::OutgoingSsnResetRequestParameter> parameter{
			RTC::SCTP::OutgoingSsnResetRequestParameter::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);

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
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Clone it. */

		parameter.reset(parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);
	}

	SECTION("OutgoingSsnResetRequestParameter::Parse() fails")
	{
		// Wrong length field (not even).
		// clang-format off
		alignas(4) const uint8_t buffer1[] =
		{
			// Type:13 (OUTGOING-SSN-RESET-REQUEST), Length: 21
			0x00, 0x0D, 0x00, 0x15,
			// Re-configuration Request Sequence Number: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Re-configuration Response Sequence Number: 0x55667788
			0x55, 0x66, 0x77, 0x88,
			// Sender's Last Assigned TSN: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
			// Stream 1: 0x5001, stream 2: 0x5002
			0x50, 0x01, 0x50, 0x02,
			// 1 extra byte, 3 bytes of padding
			0x50, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::OutgoingSsnResetRequestParameter::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("OutgoingSsnResetRequestParameter::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::OutgoingSsnResetRequestParameter> parameter{
			RTC::SCTP::OutgoingSsnResetRequestParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 0);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 0);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 0);

		std::vector<uint16_t> expectedStreamIds{};

		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Modify it. */

		parameter->SetReconfigurationRequestSequenceNumber(111000);
		parameter->SetReconfigurationResponseSequenceNumber(222000);
		parameter->SetSenderLastAssignedTsn(333000);
		parameter->AddStreamId(4444);
		parameter->AddStreamId(4445);
		parameter->AddStreamId(4446);
		parameter->AddStreamId(4447);
		parameter->AddStreamId(4448);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 28,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 222000);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 333000);

		expectedStreamIds = {
			{ 4444, 4445, 4446, 4447, 4448 }
		};

		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);

		/* Parse itself and compare. */

		parameter.reset(
		  RTC::SCTP::OutgoingSsnResetRequestParameter::Parse(
		    parameter->GetBuffer(), parameter->GetLength()));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 28,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetReconfigurationRequestSequenceNumber() == 111000);
		REQUIRE(parameter->GetReconfigurationResponseSequenceNumber() == 222000);
		REQUIRE(parameter->GetSenderLastAssignedTsn() == 333000);
		REQUIRE(parameter->GetStreamIds() == expectedStreamIds);
	}

	SECTION("OutgoingSsnResetRequestParameter::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::SCTP::OutgoingSsnResetRequestParameter::ComputeLengthFor(0) == 16);

		const std::unique_ptr<RTC::SCTP::OutgoingSsnResetRequestParameter> parameter{
			RTC::SCTP::OutgoingSsnResetRequestParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		// An odd number of stream identifiers, so the parameter needs padding.
		parameter->AddStreamId(4444);
		parameter->AddStreamId(4445);
		parameter->AddStreamId(4446);

		REQUIRE(RTC::SCTP::OutgoingSsnResetRequestParameter::ComputeLengthFor(3) == 24);
		REQUIRE(
		  parameter->GetLength() == RTC::SCTP::OutgoingSsnResetRequestParameter::ComputeLengthFor(3));
	}
}
