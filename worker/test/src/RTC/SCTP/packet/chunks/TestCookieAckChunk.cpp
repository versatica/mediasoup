#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Cookie Acknowledgement Chunk (11)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("CookieAckChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:11 (COOKIE-ACK), Flags:0x00000001, T: 1, Length: 4
			0x0B, 0b00000101, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::CookieAckChunk> chunk{ RTC::SCTP::CookieAckChunk::Parse(
			buffer, sizeof(buffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Clone it. */

		chunk.reset(chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);
	}

	SECTION("RTC::SCTP::CookieAckChunk::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::CookieAckChunk> chunk{ RTC::SCTP::CookieAckChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Parse itself and compare. */

		chunk.reset(RTC::SCTP::CookieAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength()));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);
	}
}
