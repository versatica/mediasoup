#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP Unknown Chunk", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnknownChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:0xEE (UNKNOWN), Flags: 0b1100, Length: 7
			0xEE, 0b10001100, 0x00, 0x07,
			// Unknown value: 0xAABBCC, 1 byte of padding
			0xAA, 0xBB, 0xCC, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = UnknownChunk::Parse(buffer, sizeof(buffer));

		// NOTE: Chunk Type is 0xEE (0b11101110) so first 2 bits are 11, meaning
		// that the action to take if we receive this Chunk Type is SKIP_AND_REPORT.

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasUnknownValue() == true);
		REQUIRE(chunk->GetUnknownValueLength() == 3);
		REQUIRE(chunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownValue()[2] == 0xCC);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasUnknownValue() == true);
		REQUIRE(chunk->GetUnknownValueLength() == 3);
		REQUIRE(chunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownValue()[2] == 0xCC);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->HasUnknownValue() == true);
		REQUIRE(clonedChunk->GetUnknownValueLength() == 3);
		REQUIRE(clonedChunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(clonedChunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(clonedChunk->GetUnknownValue()[2] == 0xCC);

		delete clonedChunk;
	}
}
