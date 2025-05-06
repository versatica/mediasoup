#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP Cookie Acknowledgement Chunk (11)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("CookieAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:11 (COOKIE_ACK), Flags:0x00000001, T: 1, Length: 4
			0x0B, 0b00000101, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA
		};
		// clang-format on

		auto* chunk = CookieAckChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		delete clonedChunk;
	}

	SECTION("CookieAckChunk::Factory() succeeds")
	{
		auto* chunk = CookieAckChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Parse itself and compare. */

		auto* parsedChunk = CookieAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		delete parsedChunk;
	}
}
