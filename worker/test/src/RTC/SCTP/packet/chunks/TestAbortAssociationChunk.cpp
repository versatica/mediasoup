#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Abort Association Chunk (6)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("AbortAssociationChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:6 (ABORT), Flags:0b00000000, Length: 12
			0x06, 0b00000001, 0x00, 0x0C,
			// Error Cause 3: Code:1 (STALE_COOKIE), Length: 8
			0x00, 0x03, 0x00, 0x08,
			// Measure of Staleness: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = RTC::SCTP::AbortAssociationChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(chunk->GetT() == true);

		auto* errorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		errorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		errorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(clonedChunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		delete clonedChunk;
	}

	SECTION("AbortAssociationChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::AbortAssociationChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == false);

		/* Modify it and add Error Causes. */

		chunk->SetT(true);

		auto* errorCause1 = chunk->BuildErrorCauseInPlace<RTC::SCTP::StaleCookieErrorCause>();

		errorCause1->SetMeasureOfStaleness(666);
		errorCause1->Consolidate();

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4 + (4 + 4),
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(chunk->GetT() == true);

		const auto* addedErrorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ addedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 666);

		/* Parse itself and compare. */

		auto* parsedChunk =
		  RTC::SCTP::AbortAssociationChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 4),
		  /*length*/ 4 + (4 + 4),
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(parsedChunk->GetT() == true);

		const auto* parsedErrorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(parsedChunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause1->GetMeasureOfStaleness() == 666);

		delete parsedChunk;
	}

	SECTION("AbortAssociationChunk::Factory() with AddErrorCause() succeeds")
	{
		auto* chunk = RTC::SCTP::AbortAssociationChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		chunk->SetT(true);

		// 8 bytes Error Cause.
		auto* errorCause1 = RTC::SCTP::StaleCookieErrorCause::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		errorCause1->SetMeasureOfStaleness(666666);

		chunk->AddErrorCause(errorCause1);

		// Once added, we can delete the Error Cause.
		delete errorCause1;

		// Chunk length must be:
		// - Chunk header: 4
		// - Error Cause 1: 8
		// - Total: 12

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(chunk->GetT() == true);

		auto* obtainedErrorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ obtainedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(obtainedErrorCause1->GetMeasureOfStaleness() == 666666);

		/* Parse itself and compare. */

		auto* parsedChunk =
		  RTC::SCTP::AbortAssociationChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(parsedChunk->GetT() == true);

		obtainedErrorCause1 =
		  reinterpret_cast<const RTC::SCTP::StaleCookieErrorCause*>(parsedChunk->GetErrorCauseAt(0));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ obtainedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(obtainedErrorCause1->GetMeasureOfStaleness() == 666666);

		delete parsedChunk;
	}
}
