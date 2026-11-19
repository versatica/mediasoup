#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Shutdown Association Chunk (7)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("ShutdownChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:7 (SHUTDOWN), Flags:0x00000000, Length: 8
			0x07, 0b00000000, 0x00, 0x08,
			// Cumulative TSN Ack: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::ShutdownChunk> chunk{ RTC::SCTP::ShutdownChunk::Parse(
			buffer, sizeof(buffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Clone it. */

		chunk.reset(chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);
	}

	SECTION("ShutdownChunk::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::ShutdownChunk> chunk{ RTC::SCTP::ShutdownChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0);

		/* Modify it. */

		chunk->SetCumulativeTsnAck(99887766);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 99887766);

		/* Parse itself and compare. */

		chunk.reset(RTC::SCTP::ShutdownChunk::Parse(chunk->GetBuffer(), chunk->GetLength()));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 99887766);
	}
}
