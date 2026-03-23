#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("Forward Cumulative TSN Chunk (192)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("ForwardTsnChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* chunk = RTC::SCTP::ForwardTsnChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 2);

		const std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream> expectedSkippedStreams{
			{ 4660,  17185 },
      { 22136, 34661 }
		};

		REQUIRE(chunk->GetSkippedStreams() == expectedSkippedStreams);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 2);
		REQUIRE(chunk->GetSkippedStreams() == expectedSkippedStreams);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(clonedChunk->GetNumberOfSkippedStreams() == 2);
		REQUIRE(clonedChunk->GetSkippedStreams() == expectedSkippedStreams);

		delete clonedChunk;
	}

	SECTION("ForwardTsnChunk::Parse() fails")
	{
		// Length field is not even.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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

		REQUIRE(!RTC::SCTP::ForwardTsnChunk::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("ForwardTsnChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::ForwardTsnChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 0);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 0);

		std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream> expectedSkippedStreams{};

		REQUIRE(chunk->GetSkippedStreams() == expectedSkippedStreams);

		/* Modify it. */

		chunk->SetNewCumulativeTsn(1234);
		chunk->AddStream(1111, 11110);
		chunk->AddStream(2222, 22220);
		chunk->AddStream(3333, 33330);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 1234);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 3);

		expectedSkippedStreams = {
			{ 1111, 11110 },
			{ 2222, 22220 },
			{ 3333, 33330 },
		};

		REQUIRE(chunk->GetSkippedStreams() == expectedSkippedStreams);

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::ForwardTsnChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetNewCumulativeTsn() == 1234);
		REQUIRE(parsedChunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(parsedChunk->GetSkippedStreams() == expectedSkippedStreams);

		delete parsedChunk;
	}
}
