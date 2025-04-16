#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>

using namespace RTC::SCTP;

SCENARIO("SCTP Shutdown Ack Chunk (8)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:8 (SHUTDOWN_ACK), Flags:0x00000000, Length: 4
			0x08, 0b01000000, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = ShutdownAckChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		delete clonedChunk;
	}

	SECTION("ShutdownAckChunk::Factory() succeeds")
	{
		auto* chunk = ShutdownAckChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		delete parsedChunk;
	}
}
