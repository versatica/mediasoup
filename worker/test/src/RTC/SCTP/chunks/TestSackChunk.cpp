#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/chunks/SackChunk.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>

SCENARIO("Selective Acknowledgement Chunk (3)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("SackChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:3 (SACK), Flags: 0b00000000, Length: 36
			0x03, 0b00000000, 0x00, 0x24,
			// Cumulative TSN Ack: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Advertised Receiver Window Credit: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Number of Gap Ack Blocks: 2, Number of Duplicate TSNs: 3
			0x00, 0x02, 0x00, 0x03,
			// Gap Ack Block 1: Start: 1000, End: 1999
			0x03, 0xE8, 0x07, 0xCF,
			// Gap Ack Block 2: Start: 2000, End: 2999
			0x07, 0xDE, 0x0B, 0xB7,
			// Duplicate TSN 1: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Duplicate TSN 2: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Duplicate TSN 3: 556942164
			0x21, 0x32, 0x43, 0x54,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = SackChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfGapAckBlocks() == 2);
		REQUIRE(chunk->GetNumberOfDuplicateTsns() == 3);

		// TODO: More checks.

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetCumulativeTsnAck(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetAdvertisedReceiverWindowCredit(1234), MediaSoupError);

		// /* Serialize it. */

		// chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		// checkChunk(
		//   /*chunk*/ chunk,
		//   /*buffer*/ SerializeBuffer,
		//   /*bufferLength*/ sizeof(SerializeBuffer),
		//   /*length*/ 36,
		//   /*frozen*/ false,
		//   /*chunkType*/ Chunk::ChunkType::SACK,
		//   /*unknownType*/ false,
		//   /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		//   /*flags*/ 0b00000000,
		//   /*parametersCount*/ 0);

		// REQUIRE(chunk->GetInitiateTag() == 287454020);
		// REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		// REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		// REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		// REQUIRE(chunk->GetInitialTsn() == 2882339074);

		// /* Clone it. */

		// auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		delete chunk;

		// checkChunk(
		//   /*chunk*/ clonedChunk,
		//   /*buffer*/ CloneBuffer,
		//   /*bufferLength*/ sizeof(CloneBuffer),
		//   /*length*/ 36,
		//   /*frozen*/ false,
		//   /*chunkType*/ Chunk::ChunkType::SACK,
		//   /*unknownType*/ false,
		//   /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		//   /*flags*/ 0b00000000,
		//   /*parametersCount*/ 0);

		// REQUIRE(clonedChunk->GetInitiateTag() == 287454020);
		// REQUIRE(clonedChunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		// REQUIRE(clonedChunk->GetNumberOfOutboundStreams() == 4660);
		// REQUIRE(clonedChunk->GetNumberOfInboundStreams() == 22136);
		// REQUIRE(clonedChunk->GetInitialTsn() == 2882339074);

		// delete clonedChunk;
	}

	// SECTION("SackChunk::Factory() succeeds")
	// {
	// 	auto* chunk = SackChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

	// 	checkChunk(
	// 	  /*chunk*/ chunk,
	// 	  /*buffer*/ FactoryBuffer,
	// 	  /*bufferLength*/ sizeof(FactoryBuffer),
	// 	  /*length*/ 20,
	// 	  /*frozen*/ false,
	// 	  /*chunkType*/ Chunk::ChunkType::SACK,
	// 	  /*unknownType*/ false,
	// 	  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
	// 	  /*flags*/ 0b00000000,
	// 	  /*parametersCount*/ 0);

	// 	REQUIRE(chunk->GetInitiateTag() == 0);
	// 	REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);
	// 	REQUIRE(chunk->GetNumberOfOutboundStreams() == 0);
	// 	REQUIRE(chunk->GetNumberOfInboundStreams() == 0);
	// 	REQUIRE(chunk->GetInitialTsn() == 0);

	// 	/* Modify it. */

	// 	chunk->SetInitiateTag(1111111110);
	// 	chunk->SetAdvertisedReceiverWindowCredit(2222222220);
	// 	chunk->SetNumberOfOutboundStreams(1234);
	// 	chunk->SetNumberOfInboundStreams(5678);
	// 	chunk->SetInitialTsn(3333333330);

	// 	checkChunk(
	// 	  /*chunk*/ chunk,
	// 	  /*buffer*/ FactoryBuffer,
	// 	  /*bufferLength*/ sizeof(FactoryBuffer),
	// 	  /*length*/ 56,
	// 	  /*frozen*/ false,
	// 	  /*chunkType*/ Chunk::ChunkType::SACK,
	// 	  /*unknownType*/ false,
	// 	  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
	// 	  /*flags*/ 0b00000000,
	// 	  /*parametersCount*/ 0);

	// 	REQUIRE(chunk->GetInitiateTag() == 1111111110);
	// 	REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
	// 	REQUIRE(chunk->GetNumberOfOutboundStreams() == 1234);
	// 	REQUIRE(chunk->GetNumberOfInboundStreams() == 5678);
	// 	REQUIRE(chunk->GetInitialTsn() == 3333333330);

	// 	/* Parse itself and compare. */

	// 	auto* parsedChunk = SackChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

	// 	delete chunk;

	// 	checkChunk(
	// 	  /*chunk*/ parsedChunk,
	// 	  /*buffer*/ FactoryBuffer,
	// 	  /*bufferLength*/ 56,
	// 	  /*length*/ 56,
	// 	  /*frozen*/ true,
	// 	  /*chunkType*/ Chunk::ChunkType::SACK,
	// 	  /*unknownType*/ false,
	// 	  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
	// 	  /*flags*/ 0b00000000,
	// 	  /*parametersCount*/ 0);

	// 	REQUIRE(parsedChunk->GetInitiateTag() == 1111111110);
	// 	REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
	// 	REQUIRE(parsedChunk->GetNumberOfOutboundStreams() == 1234);
	// 	REQUIRE(parsedChunk->GetNumberOfInboundStreams() == 5678);
	// 	REQUIRE(parsedChunk->GetInitialTsn() == 3333333330);

	// 	delete parsedChunk;
	// }
}
