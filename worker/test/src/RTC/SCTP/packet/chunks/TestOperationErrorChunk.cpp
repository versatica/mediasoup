#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Operation Error Chunk (9)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("OperationErrorChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:9 (OPERATION_ERROR), Flags:0b00000000, Length: 21
			0x09, 0b00000000, 0x00, 0x15,
			// Error Cause 1: Code:1 (INVALID_STREAM_IDENTIFIER), Length: 8
			0x00, 0x01, 0x00, 0x08,
			// Stream Identifier: 0x1234
			0x12, 0x34, 0x00, 0x00,
			// Error Cause 2: Code:4 (OUT_OF_RESOURCE), Length: 4
			0x00, 0x04, 0x00, 0x04,
			// Error Cause 3: Type:49159 (UNKNOWN), Length: 5
			0xC0, 0x07, 0x00, 0x05,
			// Unknown data: 0xAB, 3 bytes of padding
			0xAB, 0x00, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = OperationErrorChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 3);

		auto* errorCause1 =
		  reinterpret_cast<const InvalidStreamIdentifierErrorCause*>(chunk->GetErrorCauseAt(0));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<InvalidStreamIdentifierErrorCause>() == errorCause1);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetStreamIdentifier() == 0x1234);

		auto* errorCause2 = reinterpret_cast<const OutOfResourceErrorCause*>(chunk->GetErrorCauseAt(1));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<OutOfResourceErrorCause>() == errorCause2);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		auto* errorCause3 = reinterpret_cast<const UnknownErrorCause*>(chunk->GetErrorCauseAt(2));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnknownErrorCause>() == errorCause3);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(49159),
		  /*unknownCode*/ true);

		REQUIRE(errorCause3->HasUnknownValue() == true);
		REQUIRE(errorCause3->GetUnknownValueLength() == 1);
		REQUIRE(errorCause3->GetUnknownValue()[0] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause3->GetUnknownValue()[1] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[2] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->BuildErrorCauseInPlace<OutOfResourceErrorCause>(), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 3);

		errorCause1 =
		  reinterpret_cast<const InvalidStreamIdentifierErrorCause*>(chunk->GetErrorCauseAt(0));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<InvalidStreamIdentifierErrorCause>() == errorCause1);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetStreamIdentifier() == 0x1234);

		errorCause2 = reinterpret_cast<const OutOfResourceErrorCause*>(chunk->GetErrorCauseAt(1));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<OutOfResourceErrorCause>() == errorCause2);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		errorCause3 = reinterpret_cast<const UnknownErrorCause*>(chunk->GetErrorCauseAt(2));

		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnknownErrorCause>() == errorCause3);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(49159),
		  /*unknownCode*/ true);

		REQUIRE(errorCause3->HasUnknownValue() == true);
		REQUIRE(errorCause3->GetUnknownValueLength() == 1);
		REQUIRE(errorCause3->GetUnknownValue()[0] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause3->GetUnknownValue()[1] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[2] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 3);

		errorCause1 =
		  reinterpret_cast<const InvalidStreamIdentifierErrorCause*>(clonedChunk->GetErrorCauseAt(0));

		REQUIRE(clonedChunk->GetFirstErrorCauseOfCode<InvalidStreamIdentifierErrorCause>() == errorCause1);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownType*/ false);

		REQUIRE(errorCause1->GetStreamIdentifier() == 0x1234);

		errorCause2 = reinterpret_cast<const OutOfResourceErrorCause*>(clonedChunk->GetErrorCauseAt(1));

		REQUIRE(clonedChunk->GetFirstErrorCauseOfCode<OutOfResourceErrorCause>() == errorCause2);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		errorCause3 = reinterpret_cast<const UnknownErrorCause*>(clonedChunk->GetErrorCauseAt(2));

		REQUIRE(clonedChunk->GetFirstErrorCauseOfCode<UnknownErrorCause>() == errorCause3);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(49159),
		  /*unknownCode*/ true);

		REQUIRE(errorCause3->HasUnknownValue() == true);
		REQUIRE(errorCause3->GetUnknownValueLength() == 1);
		REQUIRE(errorCause3->GetUnknownValue()[0] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause3->GetUnknownValue()[1] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[2] == 0x00);
		REQUIRE(errorCause3->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("OperationErrorChunk::Factory() succeeds")
	{
		auto* chunk = OperationErrorChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnrecognizedChunkTypeErrorCause>() == nullptr);
		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnknownErrorCause>() == nullptr);

		/* Modify it by adding Error Causes. */

		auto* errorCause1 = chunk->BuildErrorCauseInPlace<UnrecognizedChunkTypeErrorCause>();

		// Unrecognized Chunk length is 5 so 3 bytes of padding will be added.
		errorCause1->SetUnrecognizedChunk(DataBuffer, 5);
		errorCause1->Consolidate();

		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnrecognizedChunkTypeErrorCause>() == errorCause1);

		// // Let's add another UnrecognizedChunkTypeErrorCause.
		auto* errorCause2 = chunk->BuildErrorCauseInPlace<UnrecognizedChunkTypeErrorCause>();

		// Unrecognized Chunk is 2 so 2 bytes of padding will be added.
		errorCause2->SetUnrecognizedChunk(DataBuffer, 2);
		errorCause2->Consolidate();

		// Still must return the first Error Cause.
		REQUIRE(chunk->GetFirstErrorCauseOfCode<UnrecognizedChunkTypeErrorCause>() == errorCause1);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 2);

		const auto* addedErrorCause1 =
		  reinterpret_cast<const UnrecognizedChunkTypeErrorCause*>(chunk->GetErrorCauseAt(0));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ addedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(addedErrorCause1->HasUnrecognizedChunk() == true);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunkLength() == 5);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[1] == 0x01);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[2] == 0x02);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[3] == 0x03);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[4] == 0x04);
		// These should be padding.
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[5] == 0x00);
		REQUIRE(addedErrorCause1->GetUnrecognizedChunk()[6] == 0x00);

		const auto* addedErrorCause2 =
		  reinterpret_cast<const UnrecognizedChunkTypeErrorCause*>(chunk->GetErrorCauseAt(1));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ addedErrorCause2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(addedErrorCause2->HasUnrecognizedChunk() == true);
		REQUIRE(addedErrorCause2->GetUnrecognizedChunkLength() == 2);
		REQUIRE(addedErrorCause2->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(addedErrorCause2->GetUnrecognizedChunk()[1] == 0x01);
		// These should be padding.
		REQUIRE(addedErrorCause2->GetUnrecognizedChunk()[2] == 0x00);
		REQUIRE(addedErrorCause2->GetUnrecognizedChunk()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = OperationErrorChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::OPERATION_ERROR,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ true,
		  /*errorCausesCount*/ 2);

		const auto* parsedErrorCause1 =
		  reinterpret_cast<const UnrecognizedChunkTypeErrorCause*>(parsedChunk->GetErrorCauseAt(0));

		REQUIRE(
		  parsedChunk->GetFirstErrorCauseOfCode<UnrecognizedChunkTypeErrorCause>() == parsedErrorCause1);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause1->HasUnrecognizedChunk() == true);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunkLength() == 5);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[1] == 0x01);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[2] == 0x02);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[3] == 0x03);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[5] == 0x00);
		REQUIRE(parsedErrorCause1->GetUnrecognizedChunk()[6] == 0x00);

		const auto* parsedErrorCause2 =
		  reinterpret_cast<const UnrecognizedChunkTypeErrorCause*>(parsedChunk->GetErrorCauseAt(1));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause2->HasUnrecognizedChunk() == true);
		REQUIRE(parsedErrorCause2->GetUnrecognizedChunkLength() == 2);
		REQUIRE(parsedErrorCause2->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(parsedErrorCause2->GetUnrecognizedChunk()[1] == 0x01);
		// These should be padding.
		REQUIRE(parsedErrorCause2->GetUnrecognizedChunk()[2] == 0x00);
		REQUIRE(parsedErrorCause2->GetUnrecognizedChunk()[3] == 0x00);

		delete parsedChunk;
	}
}
