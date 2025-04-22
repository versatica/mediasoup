#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/ErrorCause.hpp"
#include "RTC/SCTP/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/errorCauses/StaleCookieErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Abort Association Chunk (6)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("AbortAssociationChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
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

		auto* chunk = AbortAssociationChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(chunk->GetT() == true);

		auto* errorCause1 = reinterpret_cast<const StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->BuildErrorCauseInPlace<StaleCookieErrorCause>(), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		errorCause1 = reinterpret_cast<const StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		errorCause1 = reinterpret_cast<const StaleCookieErrorCause*>(clonedChunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 0x12345678);

		delete clonedChunk;
	}

	SECTION("AbortAssociationChunk::Factory() succeeds")
	{
		auto* chunk = AbortAssociationChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == false);

		/* Modify it and add Error Causes. */

		chunk->SetT(true);

		auto* errorCause1 = chunk->BuildErrorCauseInPlace<StaleCookieErrorCause>();

		errorCause1->SetMeasureOfStaleness(666);
		errorCause1->Consolidate();

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4 + (4 + 4),
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(chunk->GetT() == true);

		const auto* addedErrorCause1 =
		  reinterpret_cast<const StaleCookieErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ addedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause1->GetMeasureOfStaleness() == 666);

		/* Parse itself and compare. */

		auto* parsedChunk = AbortAssociationChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 4),
		  /*length*/ 4 + (4 + 4),
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::ABORT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 1);

		REQUIRE(parsedChunk->GetT() == true);

		const auto* parsedErrorCause1 =
		  reinterpret_cast<const StaleCookieErrorCause*>(parsedChunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause1->GetMeasureOfStaleness() == 666);

		delete parsedChunk;
	}
}
