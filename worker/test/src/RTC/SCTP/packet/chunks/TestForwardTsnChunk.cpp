#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Forward Cumulative TSN Chunk (192)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ForwardTsnChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:192 (FORWARD_TSN), Flags: 0b00000000, Length: 16
			0xC0, 0b00000000, 0x00, 0x10,
			// New Cumulative TSN: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 4660, Stream Sequence 1: 17185
			0x12, 0x34, 0x43, 0x21,
			// Stream 2: 22136, Stream Sequence 2: 34661
			0x56, 0x78, 0x87, 0x65,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = ForwardTsnChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfStreams() == 2);
		REQUIRE(chunk->GetStreamAt(0) == 4660);
		REQUIRE(chunk->GetStreamSequenceAt(0) == 17185);
		REQUIRE(chunk->GetStreamAt(1) == 22136);
		REQUIRE(chunk->GetStreamSequenceAt(1) == 34661);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetNewCumulativeTsn(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->AddStream(1111, 3333), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfStreams() == 2);
		REQUIRE(chunk->GetStreamAt(0) == 4660);
		REQUIRE(chunk->GetStreamSequenceAt(0) == 17185);
		REQUIRE(chunk->GetStreamAt(1) == 22136);
		REQUIRE(chunk->GetStreamSequenceAt(1) == 34661);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(clonedChunk->GetNumberOfStreams() == 2);
		REQUIRE(clonedChunk->GetStreamAt(0) == 4660);
		REQUIRE(clonedChunk->GetStreamSequenceAt(0) == 17185);
		REQUIRE(clonedChunk->GetStreamAt(1) == 22136);
		REQUIRE(clonedChunk->GetStreamSequenceAt(1) == 34661);

		delete clonedChunk;
	}

	SECTION("ForwardTsnChunk::Parse() fails")
	{
		// Length field is not even.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:192 (FORWARD_TSN), Flags: 0b00000000, Length: 14 (should be 16)
			0xC0, 0b00000000, 0x00, 0x0E,
			// New Cumulative TSN: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 4660, Stream Sequence 1: 17185
			0x12, 0x34, 0x43, 0x21,
			// Stream 2: 22136, Stream Sequence 2 (missing in Length field)
			0x56, 0x78, 0x87, 0x65,
		};
		// clang-format on

		REQUIRE(!ForwardTsnChunk::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("ForwardTsnChunk::Factory() succeeds")
	{
		auto* chunk = ForwardTsnChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 0);
		REQUIRE(chunk->GetNumberOfStreams() == 0);

		/* Modify it. */

		chunk->SetNewCumulativeTsn(1234);
		chunk->AddStream(1111, 11110);
		chunk->AddStream(2222, 22220);
		chunk->AddStream(3333, 33330);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 1234);
		REQUIRE(chunk->GetNumberOfStreams() == 3);
		REQUIRE(chunk->GetStreamAt(0) == 1111);
		REQUIRE(chunk->GetStreamSequenceAt(0) == 11110);
		REQUIRE(chunk->GetStreamAt(1) == 2222);
		REQUIRE(chunk->GetStreamSequenceAt(1) == 22220);
		REQUIRE(chunk->GetStreamAt(2) == 3333);
		REQUIRE(chunk->GetStreamSequenceAt(2) == 33330);

		/* Parse itself and compare. */

		auto* parsedChunk = ForwardTsnChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetNewCumulativeTsn() == 1234);
		REQUIRE(parsedChunk->GetNumberOfStreams() == 3);
		REQUIRE(parsedChunk->GetStreamAt(0) == 1111);
		REQUIRE(parsedChunk->GetStreamSequenceAt(0) == 11110);
		REQUIRE(parsedChunk->GetStreamAt(1) == 2222);
		REQUIRE(parsedChunk->GetStreamSequenceAt(1) == 22220);
		REQUIRE(parsedChunk->GetStreamAt(2) == 3333);
		REQUIRE(parsedChunk->GetStreamSequenceAt(2) == 33330);

		delete parsedChunk;
	}
}
