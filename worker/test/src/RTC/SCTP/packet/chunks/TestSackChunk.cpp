#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

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
			0x07, 0xD0, 0x0B, 0xB7,
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

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfGapAckBlocks() == 2);
		REQUIRE(chunk->GetNumberOfDuplicateTsns() == 3);
		REQUIRE(chunk->GetAckBlockStartAt(0) == 1000);
		REQUIRE(chunk->GetAckBlockEndAt(0) == 1999);
		REQUIRE(chunk->GetAckBlockStartAt(1) == 2000);
		REQUIRE(chunk->GetAckBlockEndAt(1) == 2999);
		REQUIRE(chunk->GetDuplicateTsnAt(0) == 287454020);
		REQUIRE(chunk->GetDuplicateTsnAt(1) == 4278216311);
		REQUIRE(chunk->GetDuplicateTsnAt(2) == 556942164);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetCumulativeTsnAck(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetAdvertisedReceiverWindowCredit(1234), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 36,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfGapAckBlocks() == 2);
		REQUIRE(chunk->GetNumberOfDuplicateTsns() == 3);
		REQUIRE(chunk->GetAckBlockStartAt(0) == 1000);
		REQUIRE(chunk->GetAckBlockEndAt(0) == 1999);
		REQUIRE(chunk->GetAckBlockStartAt(1) == 2000);
		REQUIRE(chunk->GetAckBlockEndAt(1) == 2999);
		REQUIRE(chunk->GetDuplicateTsnAt(0) == 287454020);
		REQUIRE(chunk->GetDuplicateTsnAt(1) == 4278216311);
		REQUIRE(chunk->GetDuplicateTsnAt(2) == 556942164);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 36,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(clonedChunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(clonedChunk->GetNumberOfGapAckBlocks() == 2);
		REQUIRE(clonedChunk->GetNumberOfDuplicateTsns() == 3);
		REQUIRE(clonedChunk->GetAckBlockStartAt(0) == 1000);
		REQUIRE(clonedChunk->GetAckBlockEndAt(0) == 1999);
		REQUIRE(clonedChunk->GetAckBlockStartAt(1) == 2000);
		REQUIRE(clonedChunk->GetAckBlockEndAt(1) == 2999);
		REQUIRE(clonedChunk->GetDuplicateTsnAt(0) == 287454020);
		REQUIRE(clonedChunk->GetDuplicateTsnAt(1) == 4278216311);
		REQUIRE(clonedChunk->GetDuplicateTsnAt(2) == 556942164);

		delete clonedChunk;
	}

	SECTION("SackChunk::Parse() fails")
	{
		// Length field doesn't match Number of Gap Ack Blocks + Number of
		// Duplicate TSNs.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:3 (SACK), Flags: 0b00000000, Length: 24 (should be 28)
			0x03, 0b00000000, 0x00, 0x18,
			// Cumulative TSN Ack: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Advertised Receiver Window Credit: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Number of Gap Ack Blocks: 1, Number of Duplicate TSNs: 2
			0x00, 0x01, 0x00, 0x02,
			// Gap Ack Block 1: Start: 1000, End: 1999
			0x03, 0xE8, 0x07, 0xCF,
			// Duplicate TSN 1: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Duplicate TSN 2: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!SackChunk::Parse(buffer1, sizeof(buffer1)));

		// Length field doesn't match Number of Gap Ack Blocks + Number of
		// Duplicate TSNs.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:3 (SACK), Flags: 0b00000000, Length: 32 (should be 28)
			0x03, 0b00000000, 0x00, 0x20,
			// Cumulative TSN Ack: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Advertised Receiver Window Credit: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Number of Gap Ack Blocks: 1, Number of Duplicate TSNs: 2
			0x00, 0x01, 0x00, 0x02,
			// Gap Ack Block 1: Start: 1000, End: 1999
			0x03, 0xE8, 0x07, 0xCF,
			// Duplicate TSN 1: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Duplicate TSN 2: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Duplicate TSN 3: 4278216312 (exceeds Number of Duplicate TSNs)
			0xFF, 0x00, 0x66, 0x78,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!SackChunk::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field (smaller than buffer).
		// clang-format off
		uint8_t buffer3[] =
		{
			// Type:3 (SACK), Flags: 0b00000000, Length: 24 (buffer is 20)
			0x03, 0b00000000, 0x00, 0x18,
			// Cumulative TSN Ack: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Advertised Receiver Window Credit: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Number of Gap Ack Blocks: 1, Number of Duplicate TSNs: 0
			0x00, 0x01, 0x00, 0x00,
			// Gap Ack Block 1: Start: 1000, End: 1999
			0x03, 0xE8, 0x07, 0xCF,
		};
		// clang-format on

		REQUIRE(!SackChunk::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("SackChunk::Factory() succeeds")
	{
		auto* chunk = SackChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);
		REQUIRE(chunk->GetNumberOfGapAckBlocks() == 0);
		REQUIRE(chunk->GetNumberOfDuplicateTsns() == 0);

		/* Modify it. */

		chunk->SetCumulativeTsnAck(1234);
		chunk->SetAdvertisedReceiverWindowCredit(5678);
		chunk->AddDuplicateTsn(10000000);
		chunk->AddAckBlock(10000, 10999);
		chunk->AddAckBlock(20000, 20999);
		chunk->AddDuplicateTsn(20000000);
		chunk->AddAckBlock(60000, 60999);
		chunk->AddDuplicateTsn(30000000);
		chunk->AddDuplicateTsn(40000000);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 44,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 1234);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 5678);
		REQUIRE(chunk->GetNumberOfGapAckBlocks() == 3);
		REQUIRE(chunk->GetNumberOfDuplicateTsns() == 4);
		REQUIRE(chunk->GetAckBlockStartAt(0) == 10000);
		REQUIRE(chunk->GetAckBlockEndAt(0) == 10999);
		REQUIRE(chunk->GetAckBlockStartAt(1) == 20000);
		REQUIRE(chunk->GetAckBlockEndAt(1) == 20999);
		REQUIRE(chunk->GetAckBlockStartAt(2) == 60000);
		REQUIRE(chunk->GetAckBlockEndAt(2) == 60999);
		REQUIRE(chunk->GetDuplicateTsnAt(0) == 10000000);
		REQUIRE(chunk->GetDuplicateTsnAt(1) == 20000000);
		REQUIRE(chunk->GetDuplicateTsnAt(2) == 30000000);
		REQUIRE(chunk->GetDuplicateTsnAt(3) == 40000000);

		/* Parse itself and compare. */

		auto* parsedChunk = SackChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 44,
		  /*length*/ 44,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetCumulativeTsnAck() == 1234);
		REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 5678);
		REQUIRE(parsedChunk->GetNumberOfGapAckBlocks() == 3);
		REQUIRE(parsedChunk->GetNumberOfDuplicateTsns() == 4);
		REQUIRE(parsedChunk->GetAckBlockStartAt(0) == 10000);
		REQUIRE(parsedChunk->GetAckBlockEndAt(0) == 10999);
		REQUIRE(parsedChunk->GetAckBlockStartAt(1) == 20000);
		REQUIRE(parsedChunk->GetAckBlockEndAt(1) == 20999);
		REQUIRE(parsedChunk->GetAckBlockStartAt(2) == 60000);
		REQUIRE(parsedChunk->GetAckBlockEndAt(2) == 60999);
		REQUIRE(parsedChunk->GetDuplicateTsnAt(0) == 10000000);
		REQUIRE(parsedChunk->GetDuplicateTsnAt(1) == 20000000);
		REQUIRE(parsedChunk->GetDuplicateTsnAt(2) == 30000000);
		REQUIRE(parsedChunk->GetDuplicateTsnAt(3) == 40000000);

		delete parsedChunk;
	}
}
