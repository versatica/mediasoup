#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("I-Forward Cumulative TSN Chunk (194)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("IForwardTsnChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* chunk = RTC::SCTP::IForwardTsnChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 32,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(
		  chunk->GetSkippedStreams() == std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		                                  {
                                       true,  4097,
                                       285212689, },
		                                  {
                                       false,    8194,
                                       570425378, },
		                                  {
                                       true,12291,
                                       855638067, },
    });

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 32,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(
		  chunk->GetSkippedStreams() == std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		                                  {
                                       true,  4097,
                                       285212689, },
		                                  {
                                       false,    8194,
                                       570425378, },
		                                  {
                                       true,12291,
                                       855638067, },
    });

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 32,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetNewCumulativeTsn() == 287454020);
		REQUIRE(clonedChunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(
		  clonedChunk->GetSkippedStreams() == std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		                                        {
                                             true,  4097,
                                             285212689, },
		                                        {
                                             false,    8194,
                                             570425378, },
		                                        {
                                             true,12291,
                                             855638067, },
    });

		delete clonedChunk;
	}

	SECTION("IForwardTsnChunk::Parse() fails")
	{
		// Length field is not multiple of 8.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
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

		REQUIRE(!RTC::SCTP::IForwardTsnChunk::Parse(buffer1, sizeof(buffer1)));
	}

	SECTION("IForwardTsnChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::IForwardTsnChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 0);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 0);
		REQUIRE(chunk->GetSkippedStreams().empty());

		/* Modify it. */

		chunk->SetNewCumulativeTsn(12345678);
		chunk->AddSkippedStream(
		  RTC::SCTP::AnyForwardTsnChunk::SkippedStream{
		    /*unordered*/ true, /*streamId*/ 1111, /*mid*/ 11110001 });
		chunk->AddSkippedStream(
		  RTC::SCTP::AnyForwardTsnChunk::SkippedStream{
		    /*unordered*/ false, /*streamId*/ 2222, /*mid*/ 22220002 });
		chunk->AddSkippedStream(
		  RTC::SCTP::AnyForwardTsnChunk::SkippedStream{
		    /*unordered*/ true, /*streamId*/ 3333, /*mid*/ 33330003 });

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 32,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetNewCumulativeTsn() == 12345678);
		REQUIRE(chunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(
		  chunk->GetSkippedStreams() == std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		                                  { true,  1111, 11110001 },
		                                  { false, 2222, 22220002 },
		                                  { true,  3333, 33330003 },
    });

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::IForwardTsnChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 32,
		  /*length*/ 32,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetNewCumulativeTsn() == 12345678);
		REQUIRE(parsedChunk->GetNumberOfSkippedStreams() == 3);
		REQUIRE(
		  parsedChunk->GetSkippedStreams() == std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		                                        { true,  1111, 11110001 },
		                                        { false, 2222, 22220002 },
		                                        { true,  3333, 33330003 },
    });

		delete parsedChunk;
	}
}
