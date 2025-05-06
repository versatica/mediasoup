#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("I-Forward Cumulative TSN Chunk (194)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IForwardTsnChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:194 (I_FORWARD_TSN), Flags: 0b00000000, Length: 32
			0xC2, 0b00000000, 0x00, 0x20,
			// New Cumulative TSN: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 4097, U: 1
			0x10, 0x01, 0x00, 0x01,
			// Message Identifier: 285212689
			0x11, 0x00, 0x00, 0x11,
			// Stream 2: 8194, U: 0
			0x20, 0x02, 0x00, 0x00,
			// Message Identifier: 570425378
			0x22, 0x00, 0x00, 0x22,
			// Stream 3: 12291, U: 1
			0x30, 0x03, 0x00, 0x01,
			// Message Identifier: 855638067
			0x33, 0x00, 0x00, 0x33,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = IForwardTsnChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 32,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfStreams() == 3);
		REQUIRE(chunk->GetStreamAt(0) == 4097);
		REQUIRE(chunk->GetUFlagAt(0) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(0) == 285212689);
		REQUIRE(chunk->GetStreamAt(1) == 8194);
		REQUIRE(chunk->GetUFlagAt(1) == false);
		REQUIRE(chunk->GetMessageIdentifierAt(1) == 570425378);
		REQUIRE(chunk->GetStreamAt(2) == 12291);
		REQUIRE(chunk->GetUFlagAt(2) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(2) == 855638067);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetNewCumulativeTsn(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->AddStream(1111, true, 88888888), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 32,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfStreams() == 3);
		REQUIRE(chunk->GetStreamAt(0) == 4097);
		REQUIRE(chunk->GetUFlagAt(0) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(0) == 285212689);
		REQUIRE(chunk->GetStreamAt(1) == 8194);
		REQUIRE(chunk->GetUFlagAt(1) == false);
		REQUIRE(chunk->GetMessageIdentifierAt(1) == 570425378);
		REQUIRE(chunk->GetStreamAt(2) == 12291);
		REQUIRE(chunk->GetUFlagAt(2) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(2) == 855638067);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 32,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(clonedChunk->GetNumberOfStreams() == 3);
		REQUIRE(clonedChunk->GetStreamAt(0) == 4097);
		REQUIRE(clonedChunk->GetUFlagAt(0) == true);
		REQUIRE(clonedChunk->GetMessageIdentifierAt(0) == 285212689);
		REQUIRE(clonedChunk->GetStreamAt(1) == 8194);
		REQUIRE(clonedChunk->GetUFlagAt(1) == false);
		REQUIRE(clonedChunk->GetMessageIdentifierAt(1) == 570425378);
		REQUIRE(clonedChunk->GetStreamAt(2) == 12291);
		REQUIRE(clonedChunk->GetUFlagAt(2) == true);
		REQUIRE(clonedChunk->GetMessageIdentifierAt(2) == 855638067);

		delete clonedChunk;
	}

	SECTION("IForwardTsnChunk::Parse() fails")
	{
		// Length field is not multiple of 8.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:194 (I_FORWARD_TSN), Flags: 0b00000000, Length: 20 (should be 24)
			0xC2, 0b00000000, 0x00, 0x14,
			// New Cumulative TSN: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Stream 1: 4097, U: 1
			0x10, 0x01, 0x00, 0x01,
			// Message Identifier: 285212689
			0x11, 0x00, 0x00, 0x11,
			// Stream 2: 8194, U: 0
			0x20, 0x02, 0x00, 0x00,
			// Message Identifier (missing in Length field)
			0x22, 0x00, 0x00, 0x22,
		};
		// clang-format on

		REQUIRE(!IForwardTsnChunk::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("IForwardTsnChunk::Factory() succeeds")
	{
		auto* chunk = IForwardTsnChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
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

		chunk->SetNewCumulativeTsn(12345678);
		chunk->AddStream(1111, true, 11110001);
		chunk->AddStream(2222, false, 22220002);
		chunk->AddStream(3333, true, 33330003);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 32,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 12345678);
		REQUIRE(chunk->GetNumberOfStreams() == 3);
		REQUIRE(chunk->GetStreamAt(0) == 1111);
		REQUIRE(chunk->GetUFlagAt(0) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(0) == 11110001);
		REQUIRE(chunk->GetStreamAt(1) == 2222);
		REQUIRE(chunk->GetUFlagAt(1) == false);
		REQUIRE(chunk->GetMessageIdentifierAt(1) == 22220002);
		REQUIRE(chunk->GetStreamAt(2) == 3333);
		REQUIRE(chunk->GetUFlagAt(2) == true);
		REQUIRE(chunk->GetMessageIdentifierAt(2) == 33330003);

		/* Parse itself and compare. */

		auto* parsedChunk = IForwardTsnChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 32,
		  /*length*/ 32,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetNewCumulativeTsn() == 12345678);
		REQUIRE(parsedChunk->GetNumberOfStreams() == 3);
		REQUIRE(parsedChunk->GetStreamAt(0) == 1111);
		REQUIRE(parsedChunk->GetUFlagAt(0) == true);
		REQUIRE(parsedChunk->GetMessageIdentifierAt(0) == 11110001);
		REQUIRE(parsedChunk->GetStreamAt(1) == 2222);
		REQUIRE(parsedChunk->GetUFlagAt(1) == false);
		REQUIRE(parsedChunk->GetMessageIdentifierAt(1) == 22220002);
		REQUIRE(parsedChunk->GetStreamAt(2) == 3333);
		REQUIRE(parsedChunk->GetUFlagAt(2) == true);
		REQUIRE(parsedChunk->GetMessageIdentifierAt(2) == 33330003);

		delete parsedChunk;
	}
}
