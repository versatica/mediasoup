#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Re-Config Chunk (130)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("ReConfigChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:130 (RE_CONFIG), Flags:0b00000000, Length: 38
			// NOTE: Length field must exclude the padding of the last Parameter.
			0x82, 0b00000000, 0x00, 0x26,
			// Parameter 1: Type:13 (OUTGOING_SSN_RESET_REQUEST), Length: 22
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
			// Parameter 2: Type:14 (INCOMING_SSN_RESET_REQUEST), Length: 10
			0x00, 0x0E, 0x00, 0x0A,
			// Re-configuration Request Sequence Number: 0x44332211
			0x44, 0x33, 0x22, 0x11,
			// Stream 1: 0x6001, 2 bytes of padding
			0x60, 0x01, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = RTC::SCTP::ReConfigChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 40,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* parameter1 =
		  reinterpret_cast<const RTC::SCTP::OutgoingSsnResetRequestParameter*>(chunk->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter1->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter1->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter1->GetNumberOfStreams() == 3);
		REQUIRE(parameter1->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter1->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter1->GetStreamAt(2) == 0x5003);

		auto* parameter2 =
		  reinterpret_cast<const RTC::SCTP::IncomingSsnResetRequestParameter*>(chunk->GetParameterAt(1));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/
		  RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetReconfigurationRequestSequenceNumber() == 0x44332211);
		REQUIRE(parameter2->GetNumberOfStreams() == 1);
		REQUIRE(parameter2->GetStreamAt(0) == 0x6001);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 40,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter1 =
		  reinterpret_cast<const RTC::SCTP::OutgoingSsnResetRequestParameter*>(chunk->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter1->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter1->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter1->GetNumberOfStreams() == 3);
		REQUIRE(parameter1->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter1->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter1->GetStreamAt(2) == 0x5003);

		parameter2 =
		  reinterpret_cast<const RTC::SCTP::IncomingSsnResetRequestParameter*>(chunk->GetParameterAt(1));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/
		  RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetReconfigurationRequestSequenceNumber() == 0x44332211);
		REQUIRE(parameter2->GetNumberOfStreams() == 1);
		REQUIRE(parameter2->GetStreamAt(0) == 0x6001);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 40,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter1 = reinterpret_cast<const RTC::SCTP::OutgoingSsnResetRequestParameter*>(
		  clonedChunk->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 24,
		  /*length*/ 24,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->GetReconfigurationRequestSequenceNumber() == 0x11223344);
		REQUIRE(parameter1->GetReconfigurationResponseSequenceNumber() == 0x55667788);
		REQUIRE(parameter1->GetSenderLastAssignedTsn() == 0xAABBCCDD);
		REQUIRE(parameter1->GetNumberOfStreams() == 3);
		REQUIRE(parameter1->GetStreamAt(0) == 0x5001);
		REQUIRE(parameter1->GetStreamAt(1) == 0x5002);
		REQUIRE(parameter1->GetStreamAt(2) == 0x5003);

		parameter2 = reinterpret_cast<const RTC::SCTP::IncomingSsnResetRequestParameter*>(
		  clonedChunk->GetParameterAt(1));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/
		  RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetReconfigurationRequestSequenceNumber() == 0x44332211);
		REQUIRE(parameter2->GetNumberOfStreams() == 1);
		REQUIRE(parameter2->GetStreamAt(0) == 0x6001);

		delete clonedChunk;
	}

	SECTION("ReConfigChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::ReConfigChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Modify it by adding Parameters. */

		auto* parameter1 = chunk->BuildParameterInPlace<RTC::SCTP::ReconfigurationResponseParameter>();

		// Parameter will have 20 bytes length.
		parameter1->SetReconfigurationResponseSequenceNumber(11112222);
		parameter1->SetResult(RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		parameter1->SetNextTsns(100000001, 200000002);
		parameter1->Consolidate();

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4 + 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		const auto* addedParameter1 =
		  reinterpret_cast<const RTC::SCTP::ReconfigurationResponseParameter*>(chunk->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ addedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter1->GetReconfigurationResponseSequenceNumber() == 11112222);
		REQUIRE(
		  addedParameter1->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(addedParameter1->HasNextTsns() == true);
		REQUIRE(addedParameter1->GetSenderNextTsn() == 100000001);
		REQUIRE(addedParameter1->GetReceiverNextTsn() == 200000002);

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::ReConfigChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4 + 20,
		  /*length*/ 4 + 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::RE_CONFIG,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		const auto* parsedParameter1 =
		  reinterpret_cast<const RTC::SCTP::ReconfigurationResponseParameter*>(
		    parsedChunk->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter1->GetReconfigurationResponseSequenceNumber() == 11112222);
		REQUIRE(
		  parsedParameter1->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS);
		REQUIRE(parsedParameter1->HasNextTsns() == true);
		REQUIRE(parsedParameter1->GetSenderNextTsn() == 100000001);
		REQUIRE(parsedParameter1->GetReceiverNextTsn() == 200000002);

		delete parsedChunk;
	}
}
