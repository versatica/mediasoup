#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>

using namespace RTC::SCTP;

SCENARIO("SCTP Shutdown Complete Chunk (14)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownCompleteChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:8 (SHUTDOWN_COMPLETE), Flags:0x00000001, T: 1, Length: 4
			0x0E, 0b00000001, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		auto* chunk = ShutdownCompleteChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetT(false), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(clonedChunk->GetT() == true);

		delete clonedChunk;
	}

	SECTION("ShutdownCompleteChunk::Factory() succeeds")
	{
		auto* chunk = ShutdownCompleteChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == false);

		/* Modify it. */

		chunk->SetT(true);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownCompleteChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(parsedChunk->GetT() == true);

		delete parsedChunk;
	}
}
