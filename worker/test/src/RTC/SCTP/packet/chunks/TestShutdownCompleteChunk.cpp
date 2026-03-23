#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Shutdown Complete Chunk (14)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("ShutdownCompleteChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:8 (SHUTDOWN_COMPLETE), Flags:0x00000001, T: 1, Length: 4
			0x0E, 0b00000001, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		auto* chunk = RTC::SCTP::ShutdownCompleteChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetT() == true);

		delete clonedChunk;
	}

	SECTION("ShutdownCompleteChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::ShutdownCompleteChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == false);

		/* Modify it. */

		chunk->SetT(true);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Parse itself and compare. */

		auto* parsedChunk =
		  RTC::SCTP::ShutdownCompleteChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetT() == true);

		delete parsedChunk;
	}
}
