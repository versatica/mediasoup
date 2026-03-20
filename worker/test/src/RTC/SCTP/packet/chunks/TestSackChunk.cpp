#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("Selective Acknowledgement Chunk (3)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("SackChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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
			// Notice that this is wrong since it should be merged with the first
			// Gap Ack Block.
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

		auto* chunk = RTC::SCTP::SackChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 36,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);

		const std::vector<uint32_t> expectedDuplicateTsns{
			{ 287454020, 4278216311, 556942164 },
		};
		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> expectedGapAckBlocks{
			{ 1000, 2999 },
		};

		REQUIRE(chunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(chunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 36,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(chunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 36,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetCumulativeTsnAck() == 287454020);
		REQUIRE(clonedChunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(clonedChunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(clonedChunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		delete clonedChunk;
	}

	SECTION("SackChunk::Parse() fails")
	{
		// Length field doesn't match Number of Gap Ack Blocks + Number of
		// Duplicate TSNs.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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

		REQUIRE(!RTC::SCTP::SackChunk::Parse(buffer1, sizeof(buffer1)));

		// Length field doesn't match Number of Gap Ack Blocks + Number of
		// Duplicate TSNs.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
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

		REQUIRE(!RTC::SCTP::SackChunk::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field (smaller than buffer).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
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

		REQUIRE(!RTC::SCTP::SackChunk::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("SackChunk::Factory() succeeds")
	{
		auto* chunk =
		  RTC::SCTP::SackChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);

		std::vector<uint32_t> expectedDuplicateTsns{};
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> expectedGapAckBlocks{};

		REQUIRE(chunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(chunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		/* Modify it. */

		chunk->SetCumulativeTsnAck(1234);
		chunk->SetAdvertisedReceiverWindowCredit(5678);
		chunk->AddDuplicateTsn(10000000);
		chunk->AddAckBlock(10000, 19999);
		chunk->AddAckBlock(20000, 20999);
		chunk->AddDuplicateTsn(20000000);
		chunk->AddAckBlock(60000, 60999);
		chunk->AddDuplicateTsn(30000000);
		chunk->AddDuplicateTsn(40000000);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 44,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 1234);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 5678);

		expectedDuplicateTsns = {
			{ 10000000, 20000000, 30000000, 40000000 }
		};
		expectedGapAckBlocks = {
			{ 10000, 20999 },
      { 60000, 60999 }
		};

		REQUIRE(chunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(chunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::SackChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 44,
		  /*length*/ 44,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetCumulativeTsnAck() == 1234);
		REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 5678);
		REQUIRE(parsedChunk->GetDuplicateTsns() == expectedDuplicateTsns);
		REQUIRE(parsedChunk->GetValidatedGapAckBlocks() == expectedGapAckBlocks);

		delete parsedChunk;
	}
}
